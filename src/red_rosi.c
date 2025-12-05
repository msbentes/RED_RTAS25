#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define NUM_TASKS 20
#define MAX_CAPACITY 50 // Capacidade de processamento do sistema por ciclo
#define OVERHEAD_JAMS 1.1 // JAMS tem um leve overhead por gestão cooperativa

typedef struct {
    int id;
    int computation_time;
    int deadline;
    int value; // Prioridade/Valor
} Task;

// Estrutura para armazenar resultados
typedef struct {
    double red_response_time;
    double jams_response_time;
    int red_accepted;
    int jams_accepted;
} Result;

// Função para gerar tarefas aleatórias
void generate_tasks(Task tasks[], int n) {
    for (int i = 0; i < n; i++) {
        tasks[i].id = i + 1;
        tasks[i].computation_time = (rand() % 15) + 5; // Entre 5 e 20
        tasks[i].deadline = tasks[i].computation_time + (rand() % 20);
        tasks[i].value = (rand() % 100);
    }
}

// Simulação do RED (Determinístico)
Result simulate_RED(Task tasks[], int n) {
    Result res = {0, 0, 0, 0};
    int current_load = 0;
    double total_response_time = 0;

    for (int i = 0; i < n; i++) {
        // Teste de Admissão Rígido
        if (current_load + tasks[i].computation_time <= MAX_CAPACITY) {
            current_load += tasks[i].computation_time;
            res.red_accepted++;
            // Tempo de resposta simplificado: Espera (load atual) + Computação
            total_response_time += current_load; 
        } else {
            // Rejeita a tarefa (não soma ao tempo de resposta médio)
        }
    }
    
    if (res.red_accepted > 0)
        res.red_response_time = total_response_time / res.red_accepted;
    
    return res;
}

// Simulação do JAMS (Probabilístico + Cooperativo)
Result simulate_JAMS(Task tasks[], int n) {
    Result res = {0, 0, 0, 0};
    int current_load = 0;
    double total_response_time = 0;

    for (int i = 0; i < n; i++) {
        int accepted = 0;

        // 1. Tenta admissão normal (igual ao RED)
        if (current_load + tasks[i].computation_time <= MAX_CAPACITY) {
            accepted = 1;
        } 
        // 2. Se falhar, tenta admissão Probabilística (JAMS)
        else {
            // Probabilidade baseada no valor da tarefa e sobrecarga
            double overload = (current_load + tasks[i].computation_time) - MAX_CAPACITY;
            double prob = (double)tasks[i].value / (overload * 10.0 + 1.0); // Fórmula fictícia de exemplo
            if (prob > 1.0) prob = 1.0;
            
            double r = (double)rand() / (double)RAND_MAX;
            
            if (r < prob) {
                accepted = 1;
            }
        }

        if (accepted) {
            // JAMS usa servidores cooperativos -> aumenta capacidade virtualmente,
            // mas pode ter um leve atraso (overhead) ou tempo de resposta maior devido à carga
            current_load += tasks[i].computation_time;
            res.jams_accepted++;
            
            // Simula tempo de resposta com overhead cooperativo
            total_response_time += (current_load * OVERHEAD_JAMS);
        }
    }

    if (res.jams_accepted > 0)
        res.jams_response_time = total_response_time / res.jams_accepted;

    return res;
}

// Função para desenhar gráfico de barras ASCII
void print_ascii_bar(const char* label, double value, int max_scale) {
    printf("%-5s |", label);
    int bars = (int)value;
    if (bars > max_scale) bars = max_scale;
    
    for (int i = 0; i < bars; i++) printf("█");
    printf(" %.2f ms\n", value);
}

int main() {
    srand(time(NULL));
    Task tasks[NUM_TASKS];
    
    printf("=== SIMULADOR COMPARATIVO RED vs JAMS ===\n");
    printf("Gerando %d tarefas aleatorias...\n\n", NUM_TASKS);
    generate_tasks(tasks, NUM_TASKS);

    // Rodar simulações independentes
    Result res_red = simulate_RED(tasks, NUM_TASKS);
    Result res_jams = simulate_JAMS(tasks, NUM_TASKS);

    // --- SAÍDA GRÁFICA NO TERMINAL (ASCII) ---
    printf("--- Grafico de Tempo Medio de Resposta ---\n");
    printf("(Cada '█' representa uma unidade de tempo)\n\n");

    print_ascii_bar("RED", res_red.red_response_time, 100);
    print_ascii_bar("JAMS", res_jams.jams_response_time, 100);

    printf("\n--- Estatisticas Detalhadas ---\n");
    printf("RED : Aceitou %d/%d tarefas. Tempo Medio: %.2f\n", 
            res_red.red_accepted, NUM_TASKS, res_red.red_response_time);
    printf("JAMS: Aceitou %d/%d tarefas. Tempo Medio: %.2f\n", 
            res_jams.jams_accepted, NUM_TASKS, res_jams.jams_response_time);

    // --- GERAÇÃO DE ARQUIVO CSV PARA PLOTAGEM EXTERNA ---
    FILE *f = fopen("logs/dados_comparativos.csv", "w");
    if (f) {
        fprintf(f, "Algoritmo,TarefasAceitas,TempoRespostaMedio\n");
        fprintf(f, "RED,%d,%.2f\n", res_red.red_accepted, res_red.red_response_time);
        fprintf(f, "JAMS,%d,%.2f\n", res_jams.jams_accepted, res_jams.jams_response_time);
        fclose(f);
        printf("\n[INFO] Dados exportados para 'logs/dados_comparativos.csv' para graficos em Excel/Gnuplot.\n");
    }

    return 0;
}
