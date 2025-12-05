#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define NUM_TASKS 20
#define MAX_CAPACITY 50
#define OVERHEAD_JAMS 1.1
#define NUM_RUNS 100 // Número de rodadas de simulação

typedef struct {
    int id;
    int computation_time;
    int deadline;
    int value;
} Task;

typedef struct {
    double red_response_time;
    double jams_response_time;
    int red_accepted;
    int jams_accepted;
} Result;

void generate_tasks(Task tasks[], int n) {
    for (int i = 0; i < n; i++) {
        tasks[i].id = i + 1;
        tasks[i].computation_time = (rand() % 15) + 5;
        tasks[i].deadline = tasks[i].computation_time + (rand() % 20);
        tasks[i].value = (rand() % 100);
    }
}

int load_tasks_from_csv(const char *filename, Task tasks[], int max_tasks) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Erro ao abrir %s\n", filename);
        return 0;
    }

    char line[256];
    int count = 0;

    while (fgets(line, sizeof(line), f) && count < max_tasks) {

        // Ignorar linhas vazias ou cabeçalho
        if (strstr(line, "timestamp") || strlen(line) < 3)
            continue;

        // Extrair coluna 1 → tempo de execução (segundos)
        char *token = strtok(line, ",");
        token = strtok(NULL, ",");   // segunda coluna

        if (!token) continue;

        double C_seconds = atof(token);
        double C_ms = C_seconds * 1000.0;   // converter para ms

        tasks[count].id = count + 1;
        tasks[count].computation_time = (int)C_ms;

        // modelar deadline (ex.: deadline = 5x C)
        tasks[count].deadline = (int)(5 * C_ms);

        // prioridade baseada em C
        tasks[count].value = rand() % 100;

        count++;
    }

    fclose(f);
    return count;
}

Result simulate_RED(Task tasks[], int n) {
    Result res = {0,0,0,0};
    int current_load = 0;
    double total_response_time = 0;

    for(int i=0;i<n;i++){
        if(current_load + tasks[i].computation_time <= MAX_CAPACITY){
            current_load += tasks[i].computation_time;
            res.red_accepted++;
            total_response_time += current_load;
        }
    }

    if(res.red_accepted>0) res.red_response_time = total_response_time / res.red_accepted;
    return res;
}

Result simulate_JAMS(Task tasks[], int n) {
    Result res = {0,0,0,0};
    int current_load = 0;
    double total_response_time = 0;

    for(int i=0;i<n;i++){
        int accepted = 0;
        if(current_load + tasks[i].computation_time <= MAX_CAPACITY){
            accepted = 1;
        } else {
            double overload = (current_load + tasks[i].computation_time) - MAX_CAPACITY;
            double prob = (double)tasks[i].value / (overload * 10.0 + 1.0);
            if(prob > 1.0) prob = 1.0;
            double r = (double)rand() / RAND_MAX;
            if(r < prob) accepted = 1;
        }

        if(accepted){
            current_load += tasks[i].computation_time;
            res.jams_accepted++;
            total_response_time += current_load * OVERHEAD_JAMS;
        }
    }

    if(res.jams_accepted>0) res.jams_response_time = total_response_time / res.jams_accepted;
    return res;
}

void print_ascii_bar(const char* label, double value, int max_scale){
    printf("%-5s |", label);
    int bars = (int)value;
    if(bars > max_scale) bars = max_scale;
    for(int i=0;i<bars;i++) printf("█");
    printf(" %.2f ms\n", value);
}

int main(){
    srand(time(NULL));
    Task tasks[NUM_TASKS];

    double red_resp_sum = 0, jams_resp_sum = 0;
    int red_acc_sum = 0, jams_acc_sum = 0;

    for(int run=0; run<NUM_RUNS; run++){
        // generate_tasks(tasks, NUM_TASKS);
        int n = load_tasks_from_csv("MPC_times/MPC_long_10/saved_times_long_0.csv",
                             tasks, NUM_TASKS);

        printf("Carregadas %d tarefas reais do CSV.\n", n);

        Result r_red = simulate_RED(tasks, NUM_TASKS);
        Result r_jams = simulate_JAMS(tasks, NUM_TASKS);

        red_resp_sum += r_red.red_response_time;
        jams_resp_sum += r_jams.jams_response_time;
        red_acc_sum += r_red.red_accepted;
        jams_acc_sum += r_jams.jams_accepted;
    }

    double avg_red_resp = red_resp_sum / NUM_RUNS;
    double avg_jams_resp = jams_resp_sum / NUM_RUNS;
    double avg_red_acc = (double)red_acc_sum / NUM_RUNS;
    double avg_jams_acc = (double)jams_acc_sum / NUM_RUNS;

    printf("=== SIMULACAO MEDIA %d RODADAS ===\n", NUM_RUNS);
    printf("--- Grafico de Tempo Medio de Resposta ---\n");
    print_ascii_bar("RED", avg_red_resp, 100);
    print_ascii_bar("JAMS", avg_jams_resp, 100);

    printf("\n--- Estatisticas Detalhadas ---\n");
    printf("RED : Aceitou %.2f/%d tarefas. Tempo Medio: %.2f\n", avg_red_acc, NUM_TASKS, avg_red_resp);
    printf("JAMS: Aceitou %.2f/%d tarefas. Tempo Medio: %.2f\n", avg_jams_acc, NUM_TASKS, avg_jams_resp);

    FILE *f = fopen("logs/dados_comparativos.csv", "w");
    if(f){
        fprintf(f,"Algoritmo,TarefasAceitasMedia,TempoRespostaMedia\n");
        fprintf(f,"RED,%.2f,%.2f\n", avg_red_acc, avg_red_resp);
        fprintf(f,"JAMS,%.2f,%.2f\n", avg_jams_acc, avg_jams_resp);
        fclose(f);
        printf("\n[INFO] Dados exportados para 'logs/logs/dados_comparativos.csv'\n");
    }

    return 0;
}

