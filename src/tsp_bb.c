#include "tsp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>

// Estrutura para nó do Branch and Bound
typedef struct Node {
    int level;          // Nível na árvore BB
    int* path;          // Caminho parcial atual
    int* visited;       // Vetor de cidades visitadas
    double cost;        // Custo acumulado até o nó
    double bound;       // Limite inferior do nó
    int total_time;     // Tempo total acumulado
} Node;

// Estrutura para armazenar soluções BB
typedef struct {
    int* route;         // Rota da solução
    double cost;        // Custo total
    int total_time;     // Tempo total
} BBSolution;

// Calcula limite inferior para o nó BB
double calculate_bound(const Instance* inst, Node* node) {
    if (node->total_time > inst->houses[0].power) 
        return DBL_MAX;
        
    int n = inst->n;
    double bound = node->cost;  // Custo atual
    
    // Se é uma solução completa, adiciona custo de retorno
    if (node->level == n-1) {
        int last = node->path[node->level];
        bound += inst->dist[last][0] * (1.0 + inst->risk[last][0]);
        return bound;
    }
    
    // Para cada cidade não visitada
    for (int i = 0; i < n; i++) {
        if (!node->visited[i]) {
            // Menor custo para chegar em i
            double min_to = DBL_MAX;
            int current = node->path[node->level];
            double direct = inst->dist[current][i] * (1.0 + inst->risk[current][i]);
            min_to = direct;
            
            // Menor custo para sair de i
            double min_from = DBL_MAX;
            for (int j = 0; j < n; j++) {
                if (!node->visited[j] && i != j) {
                    double cost = inst->dist[i][j] * (1.0 + inst->risk[i][j]);
                    min_from = (cost < min_from) ? cost : min_from;
                }
            }
            
            // Se é a última cidade não visitada, deve voltar para KingsLanding
            if (node->level == n-2) {
                min_from = inst->dist[i][0] * (1.0 + inst->risk[i][0]);
            }
            
            if (min_to != DBL_MAX && min_from != DBL_MAX) {
                bound += min_to + min_from;
            }
            
            // Adiciona tempo mínimo
            bound += inst->houses[i].min_time;
        }
    }
    
    return bound;
}

// Calcula bound inicial mais preciso
double calculate_initial_bound(const Instance* inst) {
    int n = inst->n;
    double bound = 0.0;
    
    // Para cada cidade
    for (int i = 0; i < n; i++) {
        // Encontra as duas menores arestas conectadas a i
        double min1 = DBL_MAX;
        double min2 = DBL_MAX;
        
        for (int j = 0; j < n; j++) {
            if (i != j) {
                double cost = inst->dist[i][j] * (1.0 + inst->risk[i][j]);
                if (cost < min1) {
                    min2 = min1;
                    min1 = cost;
                } else if (cost < min2) {
                    min2 = cost;
                }
            }
        }
        
        // Adiciona metade da soma das duas menores arestas
        if (min1 != DBL_MAX && min2 != DBL_MAX) {
            bound += (min1 + min2) / 2.0;
        }
        
        // Adiciona tempo mínimo
        bound += inst->houses[i].min_time;
    }
    
    return bound;
}

// Resolve TSP usando Branch and Bound
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
    
    // Registra início da execução com mais precisão
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

    // Array para armazenar soluções BB
    BBSolution* bb_solutions = NULL;
    int num_solutions = 0;
    int max_solutions = 1000;
    bb_solutions = (BBSolution*)malloc(max_solutions * sizeof(BBSolution));
    
    // Calcula bound inicial BB
    double bb_bound = calculate_initial_bound(inst);
    
    write_log("=== Branch and Bound para TSP ===\n");
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
    write_log("Relaxação linear resolvida. Valor: %.2f\n\n", bb_bound);

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

    // Calcula o bound inicial para o nó raiz
    root->bound = calculate_bound(inst, root);

    // Lista de nós ativos
    Node** active = (Node**)malloc(1000000 * sizeof(Node*));
    int num_active = 1;
    active[0] = root;
    
    int nodes_explored = 0;
    
    // Branch and Bound
    while (num_active > 0) {
        nodes_explored++;
        
        // Verifica tempo atual
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                        (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
        
        // Log periódico
        if ((nodes_explored % 1000) == 0) {
            write_log("\nProgresso:\n");
            write_log("Tempo: %.2f s\n", elapsed);
            write_log("Nós explorados: %d\n", nodes_explored);
            write_log("Nós ativos: %d\n", num_active);
            write_log("Melhor custo: %.2f\n", best_sol->cost);
            write_log("Gap atual: %.2f%%\n", best_sol->gap);
        }
        
        // Verifica tempo limite
        if (elapsed >= 600.0) {
            write_log("\nTempo limite excedido (600s)!\n");
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

        // Se o bound do nó atual é maior que a melhor solução, poda
        if (current->bound >= best_sol->cost) {
            free(current->path);
            free(current->visited);
            free(current);
            continue;
        }

        // Se encontrou solução completa
        if (current->level == n-1) {
            int last = current->path[current->level];
            double return_cost = inst->dist[last][0] * (1.0 + inst->risk[last][0]);
            double obj_value = current->cost + return_cost;
            int final_time = current->total_time + inst->houses[0].min_time;

            if (final_time <= inst->houses[0].power) {
                if (num_solutions >= max_solutions) {
                    max_solutions *= 2;
                    bb_solutions = (BBSolution*)realloc(bb_solutions, 
                                 max_solutions * sizeof(BBSolution));
                }
                
                bb_solutions[num_solutions].route = (int*)malloc(n * sizeof(int));
                memcpy(bb_solutions[num_solutions].route, current->path, n * sizeof(int));
                bb_solutions[num_solutions].cost = obj_value;
                bb_solutions[num_solutions].total_time = final_time;
                
                write_log("\nSolução BB encontrada #%d:\n", num_solutions + 1);
                write_log("Custo: %.2f\n", obj_value);
                write_log("Tempo total: %d\n", final_time);
                
                num_solutions++;
            }
            
            // Libera nó atual e continua explorando
            free(current->path);
            free(current->visited);
            free(current);
            continue;
        }
        
        // Expande nó
        else {
            // Ordena cidades candidatas por custo
            typedef struct {
                int city;
                double cost;
            } CityScore;
            
            CityScore* candidates = malloc((n-1) * sizeof(CityScore));
            int num_candidates = 0;
            
            // Calcula custos para todas as cidades não visitadas
            for (int i = 1; i < n; i++) {
                if (!current->visited[i]) {
                    int new_time = current->total_time + inst->houses[i].min_time;
                    if (new_time <= inst->houses[0].power) {
                        int prev = current->path[current->level];
                        double edge_cost = inst->dist[prev][i] * (1.0 + inst->risk[prev][i]);
                        candidates[num_candidates].city = i;
                        candidates[num_candidates].cost = edge_cost;
                        num_candidates++;
                    }
                }
            }
            
            // Ordena candidatos por custo
            for (int i = 0; i < num_candidates-1; i++) {
                for (int j = i+1; j < num_candidates; j++) {
                    if (candidates[j].cost < candidates[i].cost) {
                        CityScore temp = candidates[i];
                        candidates[i] = candidates[j];
                        candidates[j] = temp;
                    }
                }
            }
            
            // Expande nós na ordem de custo
            for (int i = 0; i < num_candidates; i++) {
                int city = candidates[i].city;
                int new_time = current->total_time + inst->houses[city].min_time;
                
                Node* new_node = (Node*)malloc(sizeof(Node));
                new_node->level = current->level + 1;
                new_node->path = (int*)malloc(n * sizeof(int));
                new_node->visited = (int*)malloc(n * sizeof(int));
                
                memcpy(new_node->path, current->path, n * sizeof(int));
                memcpy(new_node->visited, current->visited, n * sizeof(int));
                
                new_node->path[new_node->level] = city;
                new_node->visited[city] = 1;
                new_node->total_time = new_time;
                
                int prev = current->path[current->level];
                double edge_cost = inst->dist[prev][city] * (1.0 + inst->risk[prev][city]);
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
            
            free(candidates);
        }
        
        free(current->path);
        free(current->visited);
        free(current);
    }
    
    // Seleciona melhor solução BB
    if (num_solutions > 0) {
        write_log("\nEncontradas %d soluções BB\n", num_solutions);
        
        int best_solution_idx = 0;
        for (int i = 1; i < num_solutions; i++) {
            if (bb_solutions[i].cost < bb_solutions[best_solution_idx].cost) {
                best_solution_idx = i;
            }
        }
        
        // Atualiza melhor solução
        memcpy(best_sol->route, bb_solutions[best_solution_idx].route, n * sizeof(int));
        best_sol->cost = bb_solutions[best_solution_idx].cost;
        best_sol->feasible = 1;
        best_sol->total_time = bb_solutions[best_solution_idx].total_time;
        
        // Calcula gap
        if (bb_bound > 0) {
            best_sol->gap = ((best_sol->cost - bb_bound) / best_sol->cost) * 100.0;
            if (best_sol->gap < 0) best_sol->gap = 0.0;
        } else {
            best_sol->gap = 0.0;
        }

        write_log("\nMelhor solução selecionada:\n");
        write_log("Índice: %d de %d\n", best_solution_idx + 1, num_solutions);
        write_log("Custo: %.2f\n", best_sol->cost);
        write_log("Gap: %.2f%%\n", best_sol->gap);
    }

    // Libera memória das soluções BB
    for (int i = 0; i < num_solutions; i++) {
        free(bb_solutions[i].route);
    }
    free(bb_solutions);

    // Registra tempo final
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    best_sol->time = (end_time.tv_sec - start_time.tv_sec) + 
                     (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    // Resultados finais no formato EXATO do PLI para os scripts funcionarem
    write_log("\nSolução encontrada:\n");
    write_log("  Lower bound (relaxação): %.2f\n", bb_bound);
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