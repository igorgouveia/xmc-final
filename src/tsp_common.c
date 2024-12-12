#include "tsp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Inicialização da variável global
FILE* log_file = NULL;

// Implementação da função write_log
void write_log(const char* format, ...) {
    va_list args;
    
    // Remove o print na tela
    // va_start(args, format);
    // vprintf(format, args);
    // va_end(args);
    
    // Mantém a escrita no arquivo de log
    if (log_file) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
}

// Função para abrir arquivo de log
void open_log(const char* filename) {
    log_file = fopen(filename, "w");
    if (!log_file) {
        printf("Erro ao abrir arquivo de log: %s\n", filename);
        exit(1);
    }
}

// Função para fechar arquivo de log
void close_log() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
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

// Função para explicar viabilidade
void explain_feasibility(const Instance* inst, const Solution* sol, FILE* log_file) {
    if (!sol->feasible) {
        fprintf(log_file, "\nSolução inviável porque:\n");
        
        // Verifica se todos os nós foram visitados
        int has_repeated = 0;
        int has_missing = 0;
        int* visits = (int*)calloc(inst->n, sizeof(int));
        
        for (int i = 0; i < inst->n; i++) {
            if (sol->route[i] >= 0 && sol->route[i] < inst->n) {
                visits[sol->route[i]]++;
                if (visits[sol->route[i]] > 1) {
                    has_repeated = 1;
                }
            }
        }
        
        for (int i = 0; i < inst->n; i++) {
            if (visits[i] == 0) {
                has_missing = 1;
            }
        }
        
        if (has_repeated) {
            fprintf(log_file, "- Existem cidades repetidas na rota\n");
        }
        if (has_missing) {
            fprintf(log_file, "- Existem cidades não visitadas\n");
        }
        
        // Verifica tempo total
        int total_time = 0;
        for (int i = 0; i < inst->n; i++) {
            if (sol->route[i] >= 0 && sol->route[i] < inst->n) {
                total_time += inst->houses[sol->route[i]].min_time;
            }
        }
        
        if (total_time > inst->houses[0].power) {
            fprintf(log_file, "- Tempo total (%d) excede o poder de KingsLanding (%d)\n", 
                    total_time, inst->houses[0].power);
        }
        
        free(visits);
    } else {
        fprintf(log_file, "\nSolução é viável porque:\n");
        fprintf(log_file, "- Todas as cidades são visitadas exatamente uma vez\n");
        
        int total_time = 0;
        for (int i = 0; i < inst->n; i++) {
            total_time += inst->houses[sol->route[i]].min_time;
        }
        
        fprintf(log_file, "- Tempo total (%d) respeita o poder de KingsLanding (%d)\n",
                total_time, inst->houses[0].power);
        fprintf(log_file, "- Rota forma um ciclo válido começando e terminando em KingsLanding\n");
    }
} 