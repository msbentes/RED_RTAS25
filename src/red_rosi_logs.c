#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define NUM_TASKS 20
#define MAX_CAPACITY 50     // Capacidade de processamento do sistema por ciclo
#define OVERHEAD_JAMS 1.1   // JAMS tem um leve overhead por gestão cooperativa

typedef struct {
    int id;
    int computation_time;
    int deadline;
    int value; // Prioridade/Valor
} Task;

// Estrutura para resultados do experimento
typedef struct {
    double red_response_time;
    double jams_response_time;
    int red_accepted;
    int jams_accepted;
} Result;

// Estrutura de log por tarefa
typedef struct {
    int task_id;
    char algorithm[10];
    int accepted;
    double response_time;
    int load_before;
    int load_after;
} LogEntry;



void save_tasks_csv(const char *filename, Task tasks[], int n) {
    FILE *f = fopen(filename, "w");
    if (!f) { perror("tasks.csv"); return; }

    fprintf(f, "id,computation,deadline,value\n");
    for (int i = 0; i < n; i++) {
        fprintf(f, "%d,%d,%d,%d\n",
                tasks[i].id,
                tasks[i].computation_time,
                tasks[i].deadline,
                tasks[i].value);
    }
    fclose(f);
}

void log_schedule_csv(FILE *f, int cycle, int task_id, int remaining, int missed, int completed) {
    // Cabeçalho já deve ter sido escrito antes
    fprintf(f, "%d,%d,%d,%d,%d\n", cycle, task_id, remaining, missed, completed);
}

// Log final de estatísticas
void save_results_csv(const char *filename, int id, int computation_time, int deadline, double value) {
    FILE *f = fopen(filename, "w");
    if (!f) { perror("logs/results.csv"); return; }

    fprintf(f, "id,computation_time,deadline,value\n");
    fprintf(f, "%d,%d,%d,%.4f\n", id, computation_time, deadline, value);

    fclose(f);
}

// Geração de tarefas aleatórias
void generate_tasks(Task tasks[], int n) {
    for (int i = 0; i < n; i++) {
        tasks[i].id = i + 1;
        tasks[i].computation_time = (rand() % 15) + 5; // Entre 5 e 20
        tasks[i].deadline = tasks[i].computation_time + (rand() % 20);
        tasks[i].value = (rand() % 100);

        // Estatísticas finais
        save_results_csv("logs/results.csv",
                 tasks[i].id,
                 tasks[i].computation_time,
                 tasks[i].deadline,
                 tasks[i].value);

    }
}



// ---------------------- SIMULAÇÃO RED ------------------------
Result simulate_RED(Task tasks[], int n, FILE *logf) {
    Result res = {0, 0, 0, 0};
    int current_load = 0;
    double total_response_time = 0;

    for (int i = 0; i < n; i++) {

        int load_before = current_load;
        int accepted = 0;
        double rt = 0;

        // Teste rígido (determinístico)
        if (current_load + tasks[i].computation_time <= MAX_CAPACITY) {
            accepted = 1;
            current_load += tasks[i].computation_time;
            res.red_accepted++;

            // Tempo de resposta simples (load acumulado)
            rt = current_load;
            total_response_time += rt;
        }

        // Log RED
        fprintf(logf, "%d,RED,%d,%.6f,%d,%d\n",
                tasks[i].id, accepted, rt, load_before, current_load);
    }

    if (res.red_accepted > 0)
        res.red_response_time = total_response_time / res.red_accepted;

    return res;
}

// ---------------------- SIMULAÇÃO JAMS ------------------------
Result simulate_JAMS(Task tasks[], int n, FILE *logf) {
    Result res = {0, 0, 0, 0};
    int current_load = 0;
    double total_response_time = 0;

    for (int i = 0; i < n; i++) {

        int load_before = current_load;
        int accepted = 0;
        double rt = 0;

        // 1. Tenta admissão igual ao RED
        if (current_load + tasks[i].computation_time <= MAX_CAPACITY) {
            accepted = 1;
        }
        // 2. Admissão probabilística (JAMS)
        else {
            double overload = (current_load + tasks[i].computation_time) - MAX_CAPACITY;
            double prob = (double)tasks[i].value / (overload * 10.0 + 1.0);
            if (prob > 1.0) prob = 1.0;

            double r = (double)rand() / RAND_MAX;
            if (r < prob)
                accepted = 1;
        }

        if (accepted) {
            current_load += tasks[i].computation_time;
            res.jams_accepted++;

            rt = current_load * OVERHEAD_JAMS;
            total_response_time += rt;
        }

        // Log JAMS
        fprintf(logf, "%d,JAMS,%d,%.6f,%d,%d\n",
                tasks[i].id, accepted, rt, load_before, current_load);
    }

    if (res.jams_accepted > 0)
        res.jams_response_time = total_response_time / res.jams_accepted;

    return res;
}

// --------------------- ASCII BAR CHART ------------------------
void print_ascii_bar(const char* label, double value, int max_scale) {
    printf("%-5s |", label);
    int bars = (int)value;
    if (bars > max_scale) bars = max_scale;

    for (int i = 0; i < bars; i++) printf("█");
    printf(" %.2f ms\n", value);
}

// ---------------------------- MAIN ----------------------------
int main() {
    srand(time(NULL));

    Task tasks[NUM_TASKS];

    printf("=== SIMULADOR COMPARATIVO RED vs JAMS ===\n");
    printf("Gerando %d tarefas aleatorias...\n\n", NUM_TASKS);
    generate_tasks(tasks, NUM_TASKS);

    // Arquivo de log por tarefa
    FILE *logf = fopen("logs/log_tarefas.csv", "w");
    fprintf(logf, "task_id,algorithm,accepted,rt_ms,load_before,load_after\n");

    // Executa as simulações com logging
    Result res_red = simulate_RED(tasks, NUM_TASKS, logf);
    Result res_jams = simulate_JAMS(tasks, NUM_TASKS, logf);

    fclose(logf);

    // ASCII gráfico simples
    printf("--- Grafico de Tempo Medio de Resposta ---\n");
    printf("(Cada '█' representa uma unidade de tempo)\n\n");

    print_ascii_bar("RED", res_red.red_response_time, 100);
    print_ascii_bar("JAMS", res_jams.jams_response_time, 100);

    // Estatísticas gerais
    printf("\n--- Estatisticas Detalhadas ---\n");
    printf("RED : Aceitou %d/%d tarefas. Tempo Medio: %.2f ms\n",
           res_red.red_accepted, NUM_TASKS, res_red.red_response_time);
    printf("JAMS: Aceitou %d/%d tarefas. Tempo Medio: %.2f ms\n",
           res_jams.jams_accepted, NUM_TASKS, res_jams.jams_response_time);

    // Arquivo CSV para plotagem geral
    FILE *f = fopen("logs/dados_comparativos.csv", "w");
    if (f) {
        fprintf(f, "Algoritmo,TarefasAceitas,TempoRespostaMedio\n");
        fprintf(f, "RED,%d,%.2f\n", res_red.red_accepted, res_red.red_response_time);
        fprintf(f, "JAMS,%d,%.2f\n", res_jams.jams_accepted, res_jams.jams_response_time);
        fclose(f);

        printf("\n[INFO] Dados exportados para 'logs/dados_comparativos.csv'.\n");
        printf("[INFO] Log detalhado exportado para 'logs/log_tarefas.csv'.\n");
    }

    return 0;
}
