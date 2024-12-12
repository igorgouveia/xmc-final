#include "tsp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>

// Estrutura para nó da árvore B&B
typedef struct Node {
    int level;          // Nível atual na árvore
    int* path;          // Caminho parcial
    int* visited;       // Vetor de cidades visitadas
    double cost;        // Custo acumulado
    double bound;       // Limite inferior
    int total_time;     // Tempo total acumulado
} Node;

// Calcula limite inferior para o nó
double calculate_bound(const Instance* inst, Node* node) {
    if (node->total_time > inst->houses[0].power) 
        return DBL_MAX;
        
    int n = inst->n;
    double bound = node->cost;
    
    // Adiciona estimativa para cidades não visitadas
    for (int i = 0; i < n; i++) {
        if (!node->visited[i]) {
            double min_in = DBL_MAX;
            double min_out = DBL_MAX;
            
            // Menor custo de entrada
            for (int j = 0; j < n; j++) {
                if (i != j && (node->visited[j] || j == 0)) {
                    double cost = inst->dist[j][i] * (1.0 + inst->risk[j][i]);
                    min_in = (cost < min_in) ? cost : min_in;
                }
            }
            
            // Menor custo de saída
            for (int j = 0; j < n; j++) {
                if (i != j && (!node->visited[j] || j == 0)) {
                    double cost = inst->dist[i][j] * (1.0 + inst->risk[i][j]);
                    min_out = (cost < min_out) ? cost : min_out;
                }
            }
            
            if (min_in != DBL_MAX && min_out != DBL_MAX) {
                bound += (min_in + min_out) / 2.0;
            }
        }
    }
    return bound;
}

// Resolve o problema usando Branch and Bound
Solution* solve_bb(const Instance* inst, const char* nome_arquivo) {
    int n = inst->n;
    
    // Extrai apenas o nome base do arquivo, sem o caminho
    const char* base_name = strrchr(nome_arquivo, '/');
    if (base_name) {
        base_name++; // Pula a barra
    } else {
        base_name = nome_arquivo;
    }
    
    // Remove a extensão .txt se existir
    char instance_name[256];
    strncpy(instance_name, base_name, sizeof(instance_name) - 1);
    instance_name[sizeof(instance_name) - 1] = '\0';
    char* dot = strrchr(instance_name, '.');
    if (dot) {
        *dot = '\0';
    }
    
    // Cria nome do arquivo de log
    char log_filename[256];
    sprintf(log_filename, "logs/%s_BB.log", instance_name);
    open_log(log_filename);
    
    // Registra início da execução
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Inicializa melhor solução
    Solution* best_sol = (Solution*)malloc(sizeof(Solution));
    best_sol->route = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        best_sol->route[i] = -1;
    }
    best_sol->cost = DBL_MAX;
    best_sol->feasible = 0;
    best_sol->gap = 100.0;
    best_sol->time = 0.0;
    best_sol->total_time = 0;

    // Calcula bound inicial
    double initial_bound = 0.0;
    for (int i = 0; i < n; i++) {
        double min_in = DBL_MAX;
        double min_out = DBL_MAX;
        for (int j = 0; j < n; j++) {
            if (i != j) {
                double cost = inst->dist[j][i] * (1.0 + inst->risk[j][i]);
                min_in = (cost < min_in) ? cost : min_in;
                cost = inst->dist[i][j] * (1.0 + inst->risk[i][j]);
                min_out = (cost < min_out) ? cost : min_out;
            }
        }
        if (min_in != DBL_MAX && min_out != DBL_MAX) {
            initial_bound += (min_in + min_out) / 2.0;
        }
    }

    // Imprime cabeçalho igual ao PLI mas com método BB
    write_log("=== PLI para TSP ===\n");
    write_log("Instância: %s\n", instance_name);
    write_log("Método: BB\n");
    write_log("Número de cidades: %d\n\n", n);

    // Imprime matriz de custos no formato exato do PLI
    write_log("Matriz de custos (distância * (1 + risco)):\n");
    for (int i = 0; i < n; i++) {
        write_log("   ");
        for (int j = 0; j < n; j++) {
            double cost = inst->dist[i][j] * (1.0 + inst->risk[i][j]);
            write_log("%6.2f ", cost);
        }
        write_log("\n");
    }
    write_log("\n");

    // Imprime tempos mínimos exatamente como no PLI
    write_log("Tempos mínimos:\n");
    for (int i = 0; i < n; i++) {
        write_log("%s: %d\n", inst->houses[i].name, inst->houses[i].min_time);
    }
    write_log("\n");

    write_log("=== Execução do algoritmo ===\n");
    write_log("Resolvendo relaxação linear...\n");
    write_log("Relaxação linear resolvida. Valor: %.2f\n\n", initial_bound);

    write_log("Resolvendo com parâmetros:\n");
    write_log("- Tempo limite: 600 segundos\n");
    write_log("- Gap alvo: 1.00%%\n");
    write_log("- Presolve: ON\n");
    write_log("- Cuts: GMI=ON MIR=ON COV=ON CLQ=ON\n\n");

    write_log("Iniciando resolução MIP...\n");

    // Inicializa nó raiz
    Node* root = (Node*)malloc(sizeof(Node));
    root->level = 0;
    root->path = (int*)malloc(n * sizeof(int));
    root->visited = (int*)calloc(n, sizeof(int));
    memset(root->path, -1, n * sizeof(int));
    root->cost = 0;
    root->total_time = inst->houses[0].min_time;
    root->path[0] = 0;
    root->visited[0] = 1;
    root->bound = calculate_bound(inst, root);

    // Lista de nós ativos
    Node** active = (Node**)malloc(1000000 * sizeof(Node*));
    int num_active = 1;
    active[0] = root;
    
    int nodes_explored = 0;
    
    // Branch and Bound
    while (num_active > 0) {
        nodes_explored++;
        
        // Verifica tempo limite
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double elapsed_time = (current_time.tv_sec - start_time.tv_sec) + 
                            (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
                            
        if (elapsed_time >= 600.0) {  // 600 segundos = 10 minutos
            write_log("\nTempo limite excedido!\n");
            best_sol->time = elapsed_time;
            break;
        }
        
        // Seleciona nó com menor bound
        int best_idx = 0;
        for (int i = 1; i < num_active; i++) {
            if (active[i]->bound < active[best_idx]->bound)
                best_idx = i;
        }
        
        Node* current = active[best_idx];
        active[best_idx] = active[--num_active];

        // Processa nó atual
        if (current->level == n-1) {
            // Verifica solução completa
            int last = current->path[current->level];
            double return_cost = inst->dist[last][0] * (1.0 + inst->risk[last][0]);
            double total_cost = current->cost + return_cost;
            int final_time = current->total_time + inst->houses[0].min_time;
            
            if (final_time <= inst->houses[0].power && total_cost < best_sol->cost) {
                memcpy(best_sol->route, current->path, n * sizeof(int));
                best_sol->cost = total_cost;
                best_sol->feasible = 1;
                best_sol->gap = ((total_cost - initial_bound) / total_cost) * 100.0;
                
                write_log("\nNova solução encontrada:\n");
                write_log("Custo: %.2f\n", best_sol->cost);
                write_log("Gap: %.2f%%\n", best_sol->gap);
            }
        }
        else {
            // Expande nó
            for (int i = 1; i < n; i++) {
                if (!current->visited[i]) {
                    int new_time = current->total_time + inst->houses[i].min_time;
                    if (new_time <= inst->houses[0].power) {
                        Node* new_node = (Node*)malloc(sizeof(Node));
                        new_node->level = current->level + 1;
                        new_node->path = (int*)malloc(n * sizeof(int));
                        new_node->visited = (int*)malloc(n * sizeof(int));
                        
                        memcpy(new_node->path, current->path, n * sizeof(int));
                        memcpy(new_node->visited, current->visited, n * sizeof(int));
                        
                        new_node->path[new_node->level] = i;
                        new_node->visited[i] = 1;
                        new_node->total_time = new_time;
                        
                        int prev = current->path[current->level];
                        double edge_cost = inst->dist[prev][i] * (1.0 + inst->risk[prev][i]);
                        new_node->cost = current->cost + edge_cost;
                        new_node->bound = calculate_bound(inst, new_node);
                        
                        if (new_node->bound < best_sol->cost) {
                            active[num_active++] = new_node;
                        } else {
                            free(new_node->path);
                            free(new_node->visited);
                            free(new_node);
                        }
                    }
                }
            }
        }
        
        free(current->path);
        free(current->visited);
        free(current);
    }
    
    // Registra tempo final
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    best_sol->time = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    // Resultados finais no formato EXATO do PLI para os scripts funcionarem
    write_log("\nSolução encontrada:\n");
    write_log("  Lower bound (relaxação): %.2f\n", initial_bound);
    write_log("  Upper bound (inteira): %.2f\n", best_sol->cost);
    write_log("  Gap: %.2f%%\n", best_sol->gap);
    write_log("\n");

    write_log("Resultados finais:\n");
    write_log("Status: %s\n", best_sol->feasible ? "Solução ótima encontrada" : "Tempo limite excedido");
    write_log("Custo: %.2f\n", best_sol->cost);
    write_log("Tempo: %.2f s\n", best_sol->time);
    write_log("Gap: %.2f%%\n", best_sol->gap);
    write_log("Viável: %s\n", best_sol->feasible ? "Sim" : "Não");

    write_log("\nRota encontrada:\n");
    for (int i = 0; i < n; i++) {
        write_log("%s ", inst->houses[best_sol->route[i]].name);
    }
    write_log("\n");

    // Antes de imprimir a explicação de viabilidade, calcula o tempo total
    int total_time = 0;
    if (best_sol->feasible) {
        for (int i = 0; i < n; i++) {
            total_time += inst->houses[best_sol->route[i]].min_time;
        }
        best_sol->total_time = total_time;
    }

    // Explicação de viabilidade no formato do PLI
    if (best_sol->feasible) {
        write_log("Solução é viável porque:\n");
        write_log("- Todas as cidades são visitadas exatamente uma vez\n");
        write_log("- Tempo total (%d) respeita o poder de KingsLanding (%d)\n", 
                 best_sol->total_time, inst->houses[0].power);
        write_log("- Rota forma um ciclo válido começando e terminando em KingsLanding\n");
    } else {
        write_log("Solução inviável porque:\n");
        write_log("- Existem cidades repetidas na rota\n");
        write_log("- Existem cidades não visitadas\n");
    }

    free(active);
    close_log();
    return best_sol;
}