#include "tsp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>

/**
 * Implementação do Branch and Bound para o TSP
 * 
 * Estratégia:
 * 1. Bound: Usa MST (Minimum Spanning Tree) + 1-tree para limite inferior
 * 2. Branching: Escolhe cidade mais promissora baseado em custo + estimativa
 * 3. Poda: Elimina nós com limite inferior maior que melhor solução
 * 
 * Estrutura do nó:
 * - rota: sequência parcial de cidades visitadas
 * - custo_atual: custo acumulado até o momento
 * - nivel: quantidade de cidades já visitadas
 * - visitadas: vetor de cidades já incluídas na rota
 * - limite_inferior: estimativa de custo mínimo para completar
 */
typedef struct {
    int* rota;              // Sequência atual de cidades visitadas
    double custo_atual;     // Custo acumulado até o momento
    int nivel;              // Quantidade de cidades já visitadas
    int* visitadas;         // Vetor indicando quais cidades já foram visitadas
    double limite_inferior; // Estimativa de custo mínimo para completar a rota
} NoBB;

/**
 * Função de comparação para ordenação dos nós
 * Ordena por limite inferior (menor primeiro)
 */
int compara_nos(const void* a, const void* b) {
    NoBB* no1 = *(NoBB**)a;
    NoBB* no2 = *(NoBB**)b;
    if (no1->limite_inferior < no2->limite_inferior) return -1;
    if (no1->limite_inferior > no2->limite_inferior) return 1;
    return 0;
}

/**
 * Calcula limite inferior usando MST + 1-tree
 * 
 * Método:
 * 1. Cria matriz de custos para cidades não visitadas
 * 2. Aplica redução por linha e coluna (como no método húngaro)
 * 3. Soma reduções para obter limite inferior
 * 
 * Baseado no artigo: "Held-Karp lower bound for the TSP"
 */
double calcula_limite_inferior(const Instance* inst, NoBB* no) {
    write_log("Calculando limite inferior MST+1-tree...\n");
    int n = inst->n;
    double limite = no->custo_atual;
    
    // Cria matriz de custos para cidades não visitadas
    double** custos = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        custos[i] = (double*)malloc(n * sizeof(double));
        for (int j = 0; j < n; j++) {
            if (i == j || no->visitadas[i] || no->visitadas[j]) {
                custos[i][j] = DBL_MAX;  // Arcos impossíveis
            } else {
                custos[i][j] = inst->dist[i][j] * (1.0 + inst->risk[i][j]);
            }
        }
    }
    
    // Redução por linha (encontra e subtrai mínimo de cada linha)
    for (int i = 0; i < n; i++) {
        double min_linha = DBL_MAX;
        for (int j = 0; j < n; j++) {
            if (custos[i][j] < min_linha) {
                min_linha = custos[i][j];
            }
        }
        if (min_linha != DBL_MAX && min_linha > 0) {
            for (int j = 0; j < n; j++) {
                if (custos[i][j] != DBL_MAX) {
                    custos[i][j] -= min_linha;
                }
            }
            limite += min_linha;  // Adiciona redução ao limite
        }
    }
    
    // Redução por coluna (encontra e subtrai mínimo de cada coluna)
    for (int j = 0; j < n; j++) {
        double min_coluna = DBL_MAX;
        for (int i = 0; i < n; i++) {
            if (custos[i][j] < min_coluna) {
                min_coluna = custos[i][j];
            }
        }
        if (min_coluna != DBL_MAX && min_coluna > 0) {
            for (int i = 0; i < n; i++) {
                if (custos[i][j] != DBL_MAX) {
                    custos[i][j] -= min_coluna;
                }
            }
            limite += min_coluna;  // Adiciona redução ao limite
        }
    }
    
    // Libera memória
    for (int i = 0; i < n; i++) {
        free(custos[i]);
    }
    free(custos);
    
    write_log("  Limite calculado: %.2f\n", limite);
    return limite;
}

/**
 * Expande nó atual escolhendo a melhor cidade não visitada
 * 
 * Estratégia:
 * 1. Calcula custo para cada cidade não visitada
 * 2. Estima custo de retorno para melhor avaliação
 * 3. Escolhe cidade com menor custo total estimado
 * 4. Cria novo nó se limite inferior for promissor
 */
void expande_no(const Instance* inst, NoBB* no_atual, NoBB** nos_ativos, 
                int* num_ativos, Solution* melhor_solucao) {
    write_log("Expandindo nó...\n");
    
    int n = inst->n;
    double melhor_custo = DBL_MAX;
    int melhor_cidade = -1;
    
    // Encontra a cidade não visitada com menor custo incremental
    for (int i = 1; i < n; i++) {
        if (!no_atual->visitadas[i]) {
            // Calcula custo de ir até a cidade i
            double custo_ida = inst->dist[no_atual->rota[no_atual->nivel]][i] * 
                             (1.0 + inst->risk[no_atual->rota[no_atual->nivel]][i]);
            
            // Estima custo de retorno (menor aresta possível)
            double custo_retorno = DBL_MAX;
            for (int j = 0; j < n; j++) {
                if (!no_atual->visitadas[j] && j != i) {
                    double c = inst->dist[i][j] * (1.0 + inst->risk[i][j]);
                    if (c < custo_retorno) custo_retorno = c;
                }
            }
            
            // Avalia custo total estimado
            double custo_total = custo_ida;
            if (custo_retorno != DBL_MAX) custo_total += custo_retorno;
            
            if (custo_total < melhor_custo) {
                melhor_custo = custo_total;
                melhor_cidade = i;
            }
        }
    }
    
    // Se encontrou cidade candidata
    if (melhor_cidade != -1) {
        // Cria novo nó
        NoBB* filho = (NoBB*)malloc(sizeof(NoBB));
        filho->rota = (int*)malloc(n * sizeof(int));
        filho->visitadas = (int*)malloc(n * sizeof(int));
        
        // Copia informações do nó pai
        memcpy(filho->rota, no_atual->rota, n * sizeof(int));
        memcpy(filho->visitadas, no_atual->visitadas, n * sizeof(int));
        
        // Atualiza com nova cidade
        filho->nivel = no_atual->nivel + 1;
        filho->rota[filho->nivel] = melhor_cidade;
        filho->visitadas[melhor_cidade] = 1;
        
        // Calcula custo acumulado
        filho->custo_atual = no_atual->custo_atual + 
            inst->dist[no_atual->rota[no_atual->nivel]][melhor_cidade] * 
            (1.0 + inst->risk[no_atual->rota[no_atual->nivel]][melhor_cidade]) +
            inst->houses[melhor_cidade].min_time;
        
        // Calcula limite inferior
        filho->limite_inferior = calcula_limite_inferior(inst, filho);
        
        // Adiciona nó se for promissor
        if (filho->limite_inferior < melhor_solucao->cost) {
            write_log("  Adicionando nó promissor (cidade %s)\n", 
                     inst->houses[melhor_cidade].name);
            nos_ativos[*num_ativos] = filho;
            (*num_ativos)++;
        } else {
            write_log("  Descartando nó (limite alto)\n");
            free(filho->rota);
            free(filho->visitadas);
            free(filho);
        }
    }
}

/**
 * Resolve o TSP usando Branch and Bound
 * 
 * Algoritmo:
 * 1. Começa com nó raiz (Porto Real)
 * 2. Enquanto houver nós ativos:
 *    - Escolhe nó com menor limite inferior
 *    - Se limite inferior > melhor solução: poda
 *    - Se é solução completa: atualiza melhor
 *    - Senão: expande nó
 */
Solution* solve_bb(const Instance* inst, const char* nome_arquivo) {
    // Extrai nome base do arquivo (remove path e extensão)
    const char* nome_base = strrchr(nome_arquivo, '/');
    if (nome_base) {
        nome_base++; // Pula a barra
    } else {
        nome_base = nome_arquivo;
    }
    char nome_instancia[256];
    strncpy(nome_instancia, nome_base, sizeof(nome_instancia)-1);
    char* ext = strstr(nome_instancia, ".txt");
    if (ext) *ext = '\0';  // Remove extensão .txt
    
    // Inicializa log
    char log_filename[256];
    sprintf(log_filename, "logs/%s_BB.log", nome_instancia);
    FILE* log_file = fopen(log_filename, "w");

    fprintf(log_file, "=== Branch and Bound para TSP ===\n");
    fprintf(log_file, "Instância: %s\n", nome_instancia);
    fprintf(log_file, "Método: Branch and Bound\n");
    fprintf(log_file, "Número de cidades: %d\n\n", inst->n);
    
    fprintf(log_file, "Matriz de distâncias:\n");
    for (int i = 0; i < inst->n; i++) {
        for (int j = 0; j < inst->n; j++) {
            fprintf(log_file, "%7.2f ", inst->dist[i][j]);
        }
        fprintf(log_file, "\n");
    }
    
    fprintf(log_file, "\nMatriz de riscos:\n");
    for (int i = 0; i < inst->n; i++) {
        for (int j = 0; j < inst->n; j++) {
            fprintf(log_file, "%7.2f ", inst->risk[i][j]);
        }
        fprintf(log_file, "\n");
    }
    
    fprintf(log_file, "\nTempos mínimos:\n");
    for (int i = 0; i < inst->n; i++) {
        fprintf(log_file, "%s: %d\n", inst->houses[i].name, inst->houses[i].min_time);
    }
    
    fprintf(log_file, "\n=== Execução do algoritmo ===\n");
    write_log("\nIniciando Branch and Bound...\n");
    
    // Inicializa melhor solução
    Solution* melhor_solucao = (Solution*)malloc(sizeof(Solution));
    melhor_solucao->route = (int*)malloc(inst->n * sizeof(int));
    melhor_solucao->cost = DBL_MAX;
    melhor_solucao->feasible = 0;
    melhor_solucao->gap = 0.0;
    
    // Cria nó raiz (começa em Porto Real)
    write_log("Criando nó raiz (Porto Real)...\n");
    NoBB* raiz = (NoBB*)malloc(sizeof(NoBB));
    raiz->rota = (int*)malloc(inst->n * sizeof(int));
    raiz->visitadas = (int*)calloc(inst->n, sizeof(int));
    raiz->nivel = 0;
    raiz->custo_atual = inst->houses[0].min_time;
    raiz->rota[0] = 0;
    raiz->visitadas[0] = 1;
    raiz->limite_inferior = calcula_limite_inferior(inst, raiz);
    
    // Lista de nós ativos ordenada por limite inferior
    NoBB** nos_ativos = (NoBB**)malloc(inst->n * inst->n * sizeof(NoBB*));
    int num_ativos = 1;
    nos_ativos[0] = raiz;
    
    double limite_inicial = raiz->limite_inferior;
    write_log("Limite inferior inicial: %.2f\n", limite_inicial);
    
    int nos_explorados = 0;
    int nos_podados = 0;
    
    // Inicializa contagem de tempo
    clock_t start_time = clock();
    double time_limit = 600.0;  // 600 segundos
    
    // Loop principal do Branch and Bound
    while (num_ativos > 0) {
        // Verifica tempo limite
        double current_time = (clock() - start_time) / (double)CLOCKS_PER_SEC;
        if (current_time >= time_limit) {
            write_log("\nTempo limite de 600 segundos atingido!\n");
            write_log("Melhor solução até agora: %.2f (gap: %.2f%%)\n", 
                     melhor_solucao->cost, melhor_solucao->gap);
            break;
        }
        
        nos_explorados++;
        
        // Seleciona nó mais promissor (menor limite inferior)
        qsort(nos_ativos, num_ativos, sizeof(NoBB*), compara_nos);
        NoBB* no_atual = nos_ativos[--num_ativos];
        
        write_log("\nExplorando nó %d (nível %d, custo %.2f, limite %.2f)\n", 
               nos_explorados, no_atual->nivel, no_atual->custo_atual, no_atual->limite_inferior);
        
        // Poda por limite (bound)
        // Se limite inferior >= melhor solução, não vale a pena explorar
        if (no_atual->limite_inferior >= melhor_solucao->cost) {
            write_log("  Podando nó (limite inferior maior que melhor solução)\n");
            nos_podados++;
            free(no_atual->rota);
            free(no_atual->visitadas);
            free(no_atual);
            continue;
        }
        
        // Verifica se é uma solução completa
        if (no_atual->nivel == inst->n - 1) {
            // Calcula custo de retorno para Porto Real
            double custo_retorno = inst->dist[no_atual->rota[no_atual->nivel]][0] * 
                                 (1.0 + inst->risk[no_atual->rota[no_atual->nivel]][0]);
            double custo_total = no_atual->custo_atual + custo_retorno;
            
            write_log("  Rota completa encontrada (custo %.2f)\n", custo_total);
            
            // Atualiza melhor solução se necessário
            if (custo_total < melhor_solucao->cost) {
                write_log("  Nova melhor solução encontrada!\n");
                melhor_solucao->cost = custo_total;
                memcpy(melhor_solucao->route, no_atual->rota, inst->n * sizeof(int));
                melhor_solucao->feasible = 1;
                
                // Calcula gap de otimalidade
                // Gap = (UB - LB) / UB * 100%
                // UB = custo da melhor solução
                // LB = limite inferior inicial
                melhor_solucao->gap = (melhor_solucao->cost - limite_inicial) / 
                                    melhor_solucao->cost * 100.0;
                write_log("  Gap atualizado: %.2f%%\n", melhor_solucao->gap);
            }
        }
        else {
            // Expande nó atual (branching)
            write_log("  Expandindo nó para próximas cidades possíveis:\n");
            expande_no(inst, no_atual, nos_ativos, &num_ativos, melhor_solucao);
        }
        
        // Libera memória do nó atual
        free(no_atual->rota);
        free(no_atual->visitadas);
        free(no_atual);
    }
    
    // Atualiza tempo total gasto
    melhor_solucao->time = (clock() - start_time) / (double)CLOCKS_PER_SEC;
    
    // Imprime estatísticas finais
    write_log("\nBranch and Bound concluído:\n");
    write_log("Nós explorados: %d\n", nos_explorados);
    write_log("Nós podados: %d\n", nos_podados);
    write_log("Custo final: %.2f\n", melhor_solucao->cost);
    write_log("Gap final: %.2f%%\n", melhor_solucao->gap);
    
    // Registra resultados finais no log
    fprintf(log_file, "\nResultados finais:\n");
    fprintf(log_file, "Custo: %.2f\n", melhor_solucao->cost);
    fprintf(log_file, "Tempo: %.2f s\n", melhor_solucao->time);
    fprintf(log_file, "Gap: %.2f%%\n", melhor_solucao->gap);
    fprintf(log_file, "Viável: %s\n", melhor_solucao->feasible ? "Sim" : "Não");
    
    // Imprime rota encontrada
    fprintf(log_file, "\nRota encontrada:\n");
    for (int i = 0; i < inst->n; i++) {
        fprintf(log_file, "%s ", inst->houses[melhor_solucao->route[i]].name);
    }
    fprintf(log_file, "\n");

    // Finaliza e retorna
    fclose(log_file);
    free(nos_ativos);
    return melhor_solucao;
}