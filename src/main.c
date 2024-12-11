#include "tsp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Uso: %s arquivo_instancia\n", argv[0]);
        return 1;
    }
    
    // Lê instância
    Instance* inst = read_instance(argv[1]);
    if (!inst) {
        return 1;
    }
    
    // Resolve
    clock_t start = clock();
    Solution* sol;
    
    #ifdef USE_BB
    sol = solve_bb(inst, argv[1]);
    #else
    sol = solve_mip(inst, argv[1]);
    #endif
    
    clock_t end = clock();
    
    // Atualiza tempo
    if (sol) {
        sol->time = ((double)(end - start)) / CLOCKS_PER_SEC;
    }
    
    // Imprime resultado
    printf("Instância: %s\n", argv[1]);
    printf("Método: %s\n", 
    #ifdef USE_BB
        "Branch and Bound"
    #else
        "PLI"
    #endif
    );
    printf("Custo: %.2f\n", sol->cost);
    printf("Tempo: %.2f s\n", sol->time);
    printf("Gap: %.2f%%\n", sol->gap);
    printf("Viável: %s\n", sol->feasible ? "Sim" : "Não");
    
    printf("Rota:");
    for (int i = 0; i < inst->n; i++) {
        printf(" %s", inst->houses[sol->route[i]].name);
    }
    printf("\n");
    
    // Libera memória
    free_solution(sol);
    free_instance(inst);
    
    return 0;
} 