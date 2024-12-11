#include "tsp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <glpk.h>
#include <time.h>

/**
 * Resolve o Problema do Caixeiro Viajante usando Programação Linear Inteira
 * 
 * Formulação matemática:
 * Variáveis:
 * - x[i][j]: binária, 1 se o caminho vai da cidade i para j, 0 caso contrário
 * - u[i]: inteira, ordem da cidade i na rota (para eliminação de subciclos)
 * 
 * Função objetivo:
 * Min Σ(i,j) (dist[i][j] * (1 + risk[i][j]) + min_time[j]) * x[i][j]
 * 
 * Restrições:
 * 1. Fluxo de entrada:  Σ(i) x[i][j] = 1 para todo j
 * 2. Fluxo de saída:    Σ(j) x[i][j] = 1 para todo i
 * 3. MTZ (subciclos):   u[i] - u[j] + n*x[i][j] <= n-1 para todo i,j != 1
 */
Solution* solve_mip(const Instance* inst, const char* nome_arquivo) {
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
    
    // Inicializa arquivo de log
    char log_filename[256];
    sprintf(log_filename, "logs/%s_PLI.log", nome_instancia);
    FILE* log_file = fopen(log_filename, "w");

    // Cabeçalho do log
    fprintf(log_file, "=== PLI para TSP ===\n");
    fprintf(log_file, "Instância: %s\n", nome_instancia);
    fprintf(log_file, "Método: PLI\n");
    fprintf(log_file, "Número de cidades: %d\n\n", inst->n);
    
    // Imprime matriz de custos
    fprintf(log_file, "Matriz de custos (distância * (1 + risco)):\n");
    for (int i = 0; i < inst->n; i++) {
        for (int j = 0; j < inst->n; j++) {
            fprintf(log_file, "%7.2f ", inst->dist[i][j] * (1.0 + inst->risk[i][j]));
        }
        fprintf(log_file, "\n");
    }
    
    // Imprime tempos mínimos
    fprintf(log_file, "\nTempos mínimos:\n");
    for (int i = 0; i < inst->n; i++) {
        fprintf(log_file, "%s: %d\n", inst->houses[i].name, inst->houses[i].min_time);
    }
    
    fprintf(log_file, "\n=== Execução do algoritmo ===\n");
    
    // Inicializa estrutura de solução
    Solution* solucao = (Solution*)malloc(sizeof(Solution));
    solucao->route = (int*)malloc(inst->n * sizeof(int));
    solucao->cost = 0.0;
    solucao->feasible = 0;
    solucao->gap = 0.0;
    
    int n = inst->n;
    write_log("Número de cidades: %d\n", n);
    
    // Cria problema GLPK
    glp_prob* prob = glp_create_prob();
    glp_set_prob_name(prob, "tsp");
    glp_set_obj_dir(prob, GLP_MIN);  // Problema de minimização
    
    // Define variáveis do problema:
    // - x[i][j]: variáveis binárias para arcos
    // - u[i]: variáveis inteiras para MTZ (ordem das cidades)
    int num_vars = n * n + n;  // x[i][j] + u[i]
    glp_add_cols(prob, num_vars);
    
    // Define variáveis x[i][j] e seus custos
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int idx = i * n + j + 1;
            char name[20];
            sprintf(name, "x_%d_%d", i+1, j+1);
            glp_set_col_name(prob, idx, name);
            glp_set_col_kind(prob, idx, GLP_BV);  // Variável binária
            
            if (i != j) {  // Não permite arcos para mesma cidade
                // Custo = distância * (1 + risco) + tempo mínimo
                double custo = inst->dist[i][j] * (1.0 + inst->risk[i][j]) + 
                             inst->houses[j].min_time;
                glp_set_obj_coef(prob, idx, custo);
            }
        }
    }
    
    // Define variáveis u[i] para MTZ (eliminação de subciclos)
    for (int i = 1; i < n; i++) {
        int idx = n * n + i;
        char name[20];
        sprintf(name, "u_%d", i);
        glp_set_col_name(prob, idx, name);
        glp_set_col_kind(prob, idx, GLP_IV);  // Variável inteira
        glp_set_col_bnds(prob, idx, GLP_DB, 0.0, n-1);  // 0 <= u[i] <= n-1
    }
    
    // Define restrições:
    // - Fluxo de entrada/saída
    // - MTZ para eliminação de subciclos
    int num_rows = 2 * n + (n-1)*(n-1);  // Entrada + Saída + MTZ
    glp_add_rows(prob, num_rows);
    
    // Aloca memória para matriz esparsa
    int max_elem = num_vars * num_rows;
    int* ia = (int*)malloc((1 + max_elem) * sizeof(int));
    int* ja = (int*)malloc((1 + max_elem) * sizeof(int));
    double* ar = (double*)malloc((1 + max_elem) * sizeof(double));
    int pos = 1;
    
    // Restrições de entrada: Σ x[i][j] = 1 para todo j
    for (int j = 0; j < n; j++) {
        glp_set_row_name(prob, j+1, "in");
        glp_set_row_bnds(prob, j+1, GLP_FX, 1.0, 1.0);  // Exatamente uma entrada
        
        for (int i = 0; i < n; i++) {
            if (i != j) {
                ia[pos] = j+1;
                ja[pos] = i * n + j + 1;
                ar[pos] = 1.0;
                pos++;
            }
        }
    }
    
    // Restrições de saída: Σ x[i][j] = 1 para todo i
    for (int i = 0; i < n; i++) {
        glp_set_row_name(prob, n+i+1, "out");
        glp_set_row_bnds(prob, n+i+1, GLP_FX, 1.0, 1.0);  // Exatamente uma saída
        
        for (int j = 0; j < n; j++) {
            if (i != j) {
                ia[pos] = n+i+1;
                ja[pos] = i * n + j + 1;
                ar[pos] = 1.0;
                pos++;
            }
        }
    }
    
    // Restrições MTZ: u[i] - u[j] + n*x[i][j] <= n-1
    // Elimina subciclos usando ordem das cidades
    int row = 2*n + 1;
    for (int i = 1; i < n; i++) {
        for (int j = 1; j < n; j++) {
            if (i != j) {
                char name[30];
                sprintf(name, "mtz_%d_%d", i, j);
                glp_set_row_name(prob, row, name);
                glp_set_row_bnds(prob, row, GLP_UP, -DBL_MAX, n-1);
                
                // u[i] - u[j] + n*x[i][j] <= n-1
                ia[pos] = row;
                ja[pos] = n*n + i;
                ar[pos++] = 1.0;  // u[i]
                
                ia[pos] = row;
                ja[pos] = n*n + j;
                ar[pos++] = -1.0;  // -u[j]
                
                ia[pos] = row;
                ja[pos] = i*n + j + 1;
                ar[pos++] = n;  // n*x[i][j]
                
                row++;
            }
        }
    }
    
    // Carrega matriz de restrições
    glp_load_matrix(prob, pos-1, ia, ja, ar);
    
    // Resolve relaxação linear para bound inferior
    glp_smcp parm_lp;
    glp_init_smcp(&parm_lp);
    parm_lp.msg_lev = GLP_MSG_OFF;
    
    fprintf(log_file, "Resolvendo relaxação linear...\n");
    int err_lp = glp_simplex(prob, &parm_lp);
    double lb = 0.0;
    if (err_lp == 0) {
        lb = glp_get_obj_val(prob);
        fprintf(log_file, "Relaxação linear resolvida. Valor: %.2f\n", lb);
    } else {
        fprintf(log_file, "Erro na relaxação linear: %d\n", err_lp);
        lb = 0.0;
    }

    // Configura parâmetros do GLPK para o MIP
    glp_iocp parm;
    glp_init_iocp(&parm);
    parm.presolve = GLP_ON;       // Ativa pré-processamento
    parm.msg_lev = GLP_MSG_OFF;   // Desativa mensagens
    parm.tm_lim = 600000;         // 600 segundos (600000ms)
    parm.mip_gap = 0.01;          // Gap de 1%
    parm.br_tech = GLP_BR_PCH;    // Branching pseudocost
    parm.bt_tech = GLP_BT_BLB;    // Best local bound
    parm.pp_tech = GLP_PP_ALL;    // Preprocessamento completo
    parm.fp_heur = GLP_ON;        // Heurística feasibility pump
    parm.gmi_cuts = GLP_ON;       // Cortes Gomory
    parm.mir_cuts = GLP_ON;       // Mixed integer rounding
    parm.cov_cuts = GLP_ON;       // Cover cuts
    parm.clq_cuts = GLP_ON;       // Clique cuts

    fprintf(log_file, "\nResolvendo com parâmetros:\n");
    fprintf(log_file, "- Tempo limite: %d segundos\n", parm.tm_lim/1000);
    fprintf(log_file, "- Gap alvo: %.2f%%\n", parm.mip_gap*100);
    fprintf(log_file, "- Presolve: %s\n", parm.presolve ? "ON" : "OFF");
    fprintf(log_file, "- Cuts: GMI=%s MIR=%s COV=%s CLQ=%s\n",
           parm.gmi_cuts ? "ON" : "OFF",
           parm.mir_cuts ? "ON" : "OFF",
           parm.cov_cuts ? "ON" : "OFF",
           parm.clq_cuts ? "ON" : "OFF");

    // Resolve o MIP
    clock_t start_time = clock();
    fprintf(log_file, "\nIniciando resolução MIP...\n");
    int err = glp_intopt(prob, &parm);
    
    // Atualiza tempo total gasto
    solucao->time = (clock() - start_time) / (double)CLOCKS_PER_SEC;
    
    if (err == 0) {
        if (glp_mip_status(prob) == GLP_OPT) {
            fprintf(log_file, "Solução ótima encontrada!\n");
        } else {
            fprintf(log_file, "Solução viável encontrada (não ótima)\n");
        }
        
        solucao->feasible = 1;
        solucao->cost = glp_mip_obj_val(prob);
        
        // Reconstrói a rota a partir das variáveis x[i][j]
        fprintf(log_file, "\nRota encontrada:\n");
        int atual = 0;
        for (int i = 0; i < n; i++) {
            solucao->route[i] = atual;
            fprintf(log_file, "%d: %s\n", i+1, inst->houses[atual].name);
            for (int j = 0; j < n; j++) {
                if (atual != j && glp_mip_col_val(prob, atual * n + j + 1) > 0.5) {
                    atual = j;
                    break;
                }
            }
        }
        
        // Calcula gap usando bound da relaxação
        double ub = solucao->cost;  // Upper bound (solução inteira)
        solucao->gap = ((ub - lb) / ub) * 100.0;
        
        fprintf(log_file, "\nSolução encontrada:\n");
        fprintf(log_file, "  Lower bound (relaxação): %.2f\n", lb);
        fprintf(log_file, "  Upper bound (inteira): %.2f\n", ub);
        fprintf(log_file, "  Gap: %.2f%%\n", solucao->gap);
    } else if (solucao->time >= 600.0) {
        fprintf(log_file, "Tempo limite de 600 segundos atingido!\n");
        if (glp_mip_status(prob) == GLP_FEAS) {
            fprintf(log_file, "Solução viável encontrada antes do timeout\n");
            solucao->feasible = 1;
            solucao->cost = glp_mip_obj_val(prob);
        }
    } else {
        fprintf(log_file, "Erro na otimização MIP: %d\n", err);
    }
    
    // Libera memória
    free(ia);
    free(ja);
    free(ar);
    glp_delete_prob(prob);
    
    write_log("PLI concluído. Custo: %.2f, Gap: %.2f%%\n", 
              solucao->cost, solucao->gap);
    close_log();

    // Registra resultados finais no log
    fprintf(log_file, "\nResultados finais:\n");
    fprintf(log_file, "Custo: %.2f\n", solucao->cost);
    fprintf(log_file, "Tempo: %.2f s\n", solucao->time);
    fprintf(log_file, "Gap: %.2f%%\n", solucao->gap);
    fprintf(log_file, "Viável: %s\n", solucao->feasible ? "Sim" : "Não");
    
    fprintf(log_file, "\nRota encontrada:\n");
    for (int i = 0; i < inst->n; i++) {
        fprintf(log_file, "%s ", inst->houses[solucao->route[i]].name);
    }
    fprintf(log_file, "\n");

    fclose(log_file);
    return solucao;
} 