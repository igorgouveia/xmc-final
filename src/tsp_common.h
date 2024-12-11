#ifndef TSP_COMMON_H
#define TSP_COMMON_H

#include <stdio.h>
#include <stdarg.h>  // Para va_list, va_start, va_end

// Estruturas de dados para o Problema do Emissário de Westeros
typedef struct {
    char name[50];
    int power;
    double loyalty;
    int min_time;
} House;

typedef struct {
    int n;           // Número de casas
    House* houses;   // Vetor de casas
    double** dist;   // Matriz de distâncias
    double** risk;   // Matriz de riscos [0,1]
} Instance;

typedef struct {
    int* route;      // Rota encontrada
    double cost;     // Custo total
    int feasible;    // Solução viável?
    double gap;      // Gap de otimalidade
    double time;     // Tempo de execução (adicionado)
} Solution;

// Funções comuns
Instance* read_instance(const char* filename);
void write_solution(const char* filename, const Solution* sol, const Instance* inst);
double calculate_cost(const Instance* inst, const int* route);
void free_instance(Instance* inst);
void free_solution(Solution* sol);

// Funções de solução
Solution* solve_bb(const Instance* inst, const char* nome_arquivo);  // Branch and Bound
Solution* solve_mip(const Instance* inst, const char* nome_arquivo); // PLI

// Funções para log
void init_log(const char* instance_name, const char* method);
void write_log(const char* format, ...);
void close_log();

#endif