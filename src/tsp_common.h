#ifndef TSP_COMMON_H
#define TSP_COMMON_H

#include <stdio.h>
#include <stdarg.h>

// Estrutura para representar uma casa
typedef struct {
    char name[100];
    int power;
    double loyalty;
    int min_time;
} House;

// Estrutura para representar uma instância
typedef struct {
    int n;
    House* houses;
    double** dist;
    double** risk;
} Instance;

// Estrutura para representar uma solução
typedef struct {
    int* route;
    double cost;
    double time;
    double gap;
    int feasible;
    int total_time;
} Solution;

// Variável global para arquivo de log
FILE* log_file;  // Removido o extern

// Funções de log
void write_log(const char* format, ...);
void open_log(const char* filename);
void close_log(void);

// Funções de solução
Solution* solve_bb(const Instance* inst, const char* nome_arquivo);  // Adicionado
Solution* solve_mip(const Instance* inst, const char* nome_arquivo); // Adicionado

// Outras funções
Instance* read_instance(const char* filename);
void write_solution(const char* filename, const Solution* sol, const Instance* inst);
double calculate_cost(const Instance* inst, const int* route);
void free_instance(Instance* inst);
void free_solution(Solution* sol);

// Adicionar ao header:
void explain_feasibility(const Instance* inst, const Solution* sol, FILE* log_file);

#endif