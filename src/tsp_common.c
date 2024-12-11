#include "tsp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variáveis globais para log
static FILE* log_file = NULL;
static int logging_enabled = 0;
static int terminal_output = 0;  // Controla saída no terminal

void init_log(const char* instance_name, const char* method) {
    if (log_file) {
        fclose(log_file);
    }
    
    // Cria diretório logs se não existir
    system("mkdir -p logs");
    
    // Cria nome do arquivo de log
    char log_filename[256];
    sprintf(log_filename, "logs/%s_%s.log", instance_name, method);
    
    log_file = fopen(log_filename, "w");
    logging_enabled = (log_file != NULL);
}

void write_log(const char* format, ...) {
    va_list args;
    
    // Verifica se é mensagem inicial ou final
    terminal_output = (strstr(format, "Iniciando") || 
                      strstr(format, "Branch and Bound concluído") ||
                      strstr(format, "PLI concluído"));
    
    if (terminal_output) {
        // Escreve no terminal
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
    
    // Sempre escreve no arquivo de log
    if (logging_enabled && log_file) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
}

void close_log() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    logging_enabled = 0;
}

// Função para ler instância do arquivo
Instance* read_instance(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("Erro ao abrir arquivo %s\n", filename);
        return NULL;
    }
    
    Instance* inst = (Instance*)malloc(sizeof(Instance));
    
    // Lê número de casas
    fscanf(f, "%d", &inst->n);
    
    // Aloca memória
    inst->houses = (House*)malloc(inst->n * sizeof(House));
    inst->dist = (double**)malloc(inst->n * sizeof(double*));
    inst->risk = (double**)malloc(inst->n * sizeof(double*));
    
    for (int i = 0; i < inst->n; i++) {
        inst->dist[i] = (double*)malloc(inst->n * sizeof(double));
        inst->risk[i] = (double*)malloc(inst->n * sizeof(double));
    }
    
    // Lê dados das casas
    for (int i = 0; i < inst->n; i++) {
        char name[100];
        fscanf(f, "%s %d %lf %d", 
            name,
            &inst->houses[i].power,
            &inst->houses[i].loyalty,
            &inst->houses[i].min_time);
            
        // Copia o nome usando strncpy
        strncpy(inst->houses[i].name, name, sizeof(inst->houses[i].name) - 1);
        inst->houses[i].name[sizeof(inst->houses[i].name) - 1] = '\0';
    }
    
    // Lê matriz de distâncias
    for (int i = 0; i < inst->n; i++) {
        for (int j = 0; j < inst->n; j++) {
            fscanf(f, "%lf", &inst->dist[i][j]);
        }
    }
    
    // Lê matriz de riscos
    for (int i = 0; i < inst->n; i++) {
        for (int j = 0; j < inst->n; j++) {
            fscanf(f, "%lf", &inst->risk[i][j]);
        }
    }
    
    fclose(f);
    return inst;
}

// Função para escrever solução em arquivo
void write_solution(const char* filename, const Solution* sol, const Instance* inst) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("Erro ao criar arquivo %s\n", filename);
        return;
    }
    
    fprintf(f, "%.2f\n", sol->cost);  // Custo total
    fprintf(f, "%.2f\n", sol->gap);   // Gap de otimalidade
    fprintf(f, "%.2f\n", sol->time);  // Tempo de execução
    fprintf(f, "%d\n", sol->feasible);// Indica se é viável
    
    // Escreve rota
    for (int i = 0; i < inst->n; i++) {
        fprintf(f, "%d ", sol->route[i]);
    }
    fprintf(f, "\n");
    
    fclose(f);
}

// Função para calcular custo de uma rota
double calculate_cost(const Instance* inst, const int* route) {
    double cost = 0.0;
    
    // Soma custos das arestas
    for (int i = 0; i < inst->n-1; i++) {
        int from = route[i];
        int to = route[i+1];
        cost += inst->dist[from][to] * (1.0 + inst->risk[from][to]);
    }
    
    // Adiciona retorno a Porto Real
    int last = route[inst->n-1];
    cost += inst->dist[last][0] * (1.0 + inst->risk[last][0]);
    
    // Adiciona tempos mínimos de cada casa
    for (int i = 0; i < inst->n; i++) {
        cost += inst->houses[route[i]].min_time;
    }
    
    return cost;
}

// Função para liberar memória da instância
void free_instance(Instance* inst) {
    if (!inst) return;
    
    // Libera vetores
    free(inst->houses);
    
    // Libera matrizes
    for (int i = 0; i < inst->n; i++) {
        free(inst->dist[i]);
        free(inst->risk[i]);
    }
    free(inst->dist);
    free(inst->risk);
    
    free(inst);
}

// Função para liberar memória da solução
void free_solution(Solution* sol) {
    if (!sol) return;
    free(sol->route);
    free(sol);
} 