#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#ifndef DEFAULT_NUM_RUNS
#define DEFAULT_NUM_RUNS 100
#endif

#ifndef DEFAULT_DEADLINE_MS
#define DEFAULT_DEADLINE_MS 80  // deadline padrão quando não inferido
#endif

#ifndef DEFAULT_MAX_CAPACITY_MS
#define DEFAULT_MAX_CAPACITY_MS 50  // capacidade por ciclo (ms) usada no modelo simples
#endif

#ifndef OVERHEAD_JAMS
#define OVERHEAD_JAMS 1.1
#endif

typedef struct {
    int id;
    double computation_ms; // usando double para precisão
    double deadline_ms;
    int value;
} Task;

typedef struct {
    double red_response_time;
    double jams_response_time;
    int red_accepted;
    int jams_accepted;
} Result;

/**
 * Lê o CSV e preenche o vetor tasks.
 * Espera o CSV com pelo menos 1 coluna (tempo C em segundos).
 * Se houver mais colunas, apenas a primeira é usada.
 *
 * Retorna o número de tarefas carregadas (0 se erro).
 */
int load_tasks_from_csv(const char *filename, Task **out_tasks) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Erro ao abrir %s\n", filename);
        return 0;
    }

    char line[1024];
    int capacity = 1024; // capacidade inicial
    int count = 0;
    Task *tasks = malloc(sizeof(Task) * capacity);
    if (!tasks) {
        fclose(f);
        fprintf(stderr, "Erro de alocacao\n");
        return 0;
    }

    while (fgets(line, sizeof(line), f)) {
        // Trim leading spaces
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        // Ignorar linhas vazias ou que pareçam cabeçalho
        if (*p == '\0' || *p == '\n' || *p == '\r') continue;
        if (strncasecmp(p, "timestamp", 9) == 0) continue;
        if (p[0] == '#') continue; // comentário

        // pegar a primeira token - tempo em segundos
        char *token = strtok(p, ",;\t\n\r");
        if (!token) continue;

        double C_seconds = atof(token);
        if (C_seconds <= 0.0) continue;

        if (count >= capacity) {
            capacity *= 2;
            Task *tmp = realloc(tasks, sizeof(Task) * capacity);
            if (!tmp) { free(tasks); fclose(f); fprintf(stderr, "Erro realloc\n"); return 0; }
            tasks = tmp;
        }

        tasks[count].id = count + 1;
        // tasks[count].computation_ms = C_seconds * 1000.0; // converter para ms
        tasks[count].computation_ms = C_seconds / 1000.0;  // µs → ms

        tasks[count].deadline_ms = -1.0; // ainda indefinido (vai ser setado depois)
        tasks[count].value = rand() % 100;

        count++;
    }

    fclose(f);

    // compactar
    *out_tasks = tasks;
    return count;
}

/**
 * Ajusta deadlines quando não fornecidos: configura deadline_ms para default_deadline_ms.
 * Se preferir outra heurística (ex.: 5*C), substitua aqui.
 */
void assign_deadlines(Task tasks[], int n, double default_deadline_ms) {
    for (int i = 0; i < n; ++i) {
        if (tasks[i].deadline_ms <= 0.0) {
            // Use deadline fixo ou escala por C. Aqui usamos fixo (configurável).
            tasks[i].deadline_ms = default_deadline_ms;
            // alternativa: tasks[i].deadline_ms = tasks[i].computation_ms * 5.0;
        }
    }
}

/**
 * RED real baseado em EDF: aceita se soma(C_i / D_i) <= 1
 *
 * Retorna Result preenchido (tempo de resposta aproximado e contagem aceitas).
 * Modelo simples de tempo de resposta: R ~= C * (1 + U) onde U é utilizacao após aceitar.
 */
Result simulate_RED(Task tasks[], int n) {
    Result res = {0.0, 0.0, 0, 0};
    double utilization = 0.0;
    double total_rt = 0.0;

    for (int i = 0; i < n; ++i) {
        double C = tasks[i].computation_ms;
        double D = tasks[i].deadline_ms;
        if (D <= 0.0) continue; // segurança

        double new_util = utilization + (C / D);
        if (new_util <= 1.0 + 1e-12) {
            // aceita
            utilization = new_util;
            res.red_accepted++;
            double response_time = C * (1.0 + utilization); // modelo aproximado
            total_rt += response_time;
        } else {
            // rejeita
        }
    }

    if (res.red_accepted > 0) res.red_response_time = total_rt / res.red_accepted;
    return res;
}

/**
 * JAMS: tentativa determinística, se falhar tenta probabilística com base no value e overload.
 * current_load e MAX_CAPACITY são interpretados em ms.
 * Overhead é aplicado ao tempo de resposta.
 */
Result simulate_JAMS(Task tasks[], int n, double MAX_CAPACITY_MS) {
    Result res = {0.0, 0.0, 0, 0};
    double current_load = 0.0;
    double total_rt = 0.0;

    for (int i = 0; i < n; ++i) {
        int accepted = 0;
        double C = tasks[i].computation_ms;
        if (current_load + C <= MAX_CAPACITY_MS + 1e-9) {
            accepted = 1;
        } else {
            double overload = (current_load + C) - MAX_CAPACITY_MS;
            if (overload < 0.0) overload = 0.0;
            // fórmula de probabilidade estabilizada:
            // maiores value => maior chance; maior overload => menor chance
            double val = (double)tasks[i].value;
            double prob = val / (val + overload * 10.0 + 1.0);
            if (prob < 0.0) prob = 0.0;
            if (prob > 1.0) prob = 1.0;
            double r = (double)rand() / (RAND_MAX + 1.0);
            if (r < prob) accepted = 1;
        }

        if (accepted) {
            current_load += C;
            res.jams_accepted++;
            total_rt += current_load * OVERHEAD_JAMS;
        }
    }

    if (res.jams_accepted > 0) res.jams_response_time = total_rt / res.jams_accepted;
    return res;
}

void print_ascii_bar(const char* label, double value, int max_scale) {
    printf("%-5s |", label);
    int bars = (int)(value + 0.5);
    if (bars > max_scale) bars = max_scale;
    for (int i = 0; i < bars; ++i) printf("█");
    printf(" %.2f ms\n", value);
}

void usage(const char *prog) {
    printf("Uso: %s <csv_path> [num_runs] [deadline_ms] [max_capacity_ms]\n", prog);
    printf("Exemplo: %s MPC_times/MPC_long_10/saved_times_long_0.csv 100 80 50\n", prog);
}


Result simulate_RED_logged(Task tasks[], int n, FILE *log_task) {
    Result res = {0.0, 0.0, 0, 0};
    double utilization = 0.0;
    double total_rt = 0.0;

    for (int i = 0; i < n; ++i) {
        double C = tasks[i].computation_ms;
        double D = tasks[i].deadline_ms;

        double load_before = utilization;
        int accepted = 0;
        double rt = 0.0;

        double new_util = utilization + (C / D);

        if (new_util <= 1.0) {
            accepted = 1;
            utilization = new_util;
            res.red_accepted++;
            rt = C * (1.0 + utilization);
            total_rt += rt;
        }

        double load_after = utilization;

        // registrar tarefa
        fprintf(log_task, "%d,RED,%d,%.5f,%.5f,%.5f\n",
                i + 1, accepted, rt, load_before, load_after);
    }

    if (res.red_accepted > 0) {
        res.red_response_time = total_rt / res.red_accepted;
    }
    return res;
}

Result simulate_JAMS_logged(Task tasks[], int n, double MAX_CAPACITY_MS, FILE *log_task) {
    Result res = {0.0, 0.0, 0, 0};
    double current_load = 0.0;
    double total_rt = 0.0;

    for (int i = 0; i < n; ++i) {
        double C = tasks[i].computation_ms;

        double load_before = current_load;
        int accepted = 0;
        double rt = 0.0;

        if (current_load + C <= MAX_CAPACITY_MS) {
            accepted = 1;
        } else {
            double overload = (current_load + C) - MAX_CAPACITY_MS;
            double val = tasks[i].value;
            double prob = val / (val + overload * 10.0 + 1.0);

            double r = (double)rand() / (RAND_MAX + 1.0);
            if (r < prob) accepted = 1;
        }

        if (accepted) {
            current_load += C;
            rt = current_load * OVERHEAD_JAMS;
            res.jams_accepted++;
            total_rt += rt;
        }

        double load_after = current_load;

        fprintf(log_task, "%d,JAMS,%d,%.5f,%.5f,%.5f\n",
                i + 1, accepted, rt, load_before, load_after);
    }

    if (res.jams_accepted > 0) {
        res.jams_response_time = total_rt / res.jams_accepted;
    }
    return res;
}


int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *csv_path = argv[1];
    int num_runs = (argc >= 3) ? atoi(argv[2]) : DEFAULT_NUM_RUNS;
    double default_deadline_ms = (argc >= 4) ? atof(argv[3]) : DEFAULT_DEADLINE_MS;
    double max_capacity_ms = (argc >= 5) ? atof(argv[4]) : DEFAULT_MAX_CAPACITY_MS;

    if (num_runs <= 0) num_runs = DEFAULT_NUM_RUNS;

    Task *tasks = NULL;
    int total_tasks = load_tasks_from_csv(csv_path, &tasks);
    if (total_tasks <= 0) {
        fprintf(stderr, "Nenhuma tarefa carregada. Verifique o CSV.\n");
        return 2;
    }

    // Aqui usamos todas as tarefas lidas. Se preferir limitar, ajuste total_tasks.
    assign_deadlines(tasks, total_tasks, default_deadline_ms);

    double red_resp_sum = 0.0, jams_resp_sum = 0.0;
    long red_acc_sum = 0, jams_acc_sum = 0;

    printf("Simulador RED vs JAMS\n");
    printf("CSV: %s -> %d tarefas lidas\n", csv_path, total_tasks);
    printf("Rodadas: %d, deadline(ms) default: %.1f, max_capacity(ms): %.1f\n\n",
           num_runs, default_deadline_ms, max_capacity_ms);

    FILE *f_runs = fopen("logs/log_runs.csv", "w");
    FILE *f_tasks = fopen("logs/log_tasks.csv", "w");

    fprintf(f_runs, "run,algorithm,accepted,time_ms\n");
    fprintf(f_tasks, "task_id,algorithm,accepted,rt_ms,load_before,load_after\n");

    // Realiza num_runs rodadas para obter média (JAMS é probabilístico)
    for (int run = 0; run < num_runs; ++run) {
        // podem reembaralhar valores para variar ordens — opcional:
        // shuffle(tasks, total_tasks); // se implementar shuffle

        // Result r_red = simulate_RED(tasks, total_tasks);
        // Result r_jams = simulate_JAMS(tasks, total_tasks, max_capacity_ms);

        Result r_red = simulate_RED_logged(tasks, total_tasks, f_tasks);
        Result r_jams = simulate_JAMS_logged(tasks, total_tasks, max_capacity_ms, f_tasks);

        fprintf(f_runs, "%d,RED,%d,%.5f\n", run, r_red.red_accepted, r_red.red_response_time);
        fprintf(f_runs, "%d,JAMS,%d,%.5f\n", run, r_jams.jams_accepted, r_jams.jams_response_time);

        red_resp_sum += r_red.red_response_time;
        jams_resp_sum += r_jams.jams_response_time;
        red_acc_sum += r_red.red_accepted;
        jams_acc_sum += r_jams.jams_accepted;
    }

    fclose(f_runs);
    fclose(f_tasks);

    double avg_red_resp = red_resp_sum / num_runs;
    double avg_jams_resp = jams_resp_sum / num_runs;
    double avg_red_acc = (double)red_acc_sum / num_runs;
    double avg_jams_acc = (double)jams_acc_sum / num_runs;

    printf("=== RESULTADOS MEDIOS (%d rodadas) ===\n", num_runs);
    printf("--- Grafico de Tempo Medio de Resposta ---\n");
    print_ascii_bar("RED", avg_red_resp, 100);
    print_ascii_bar("JAMS", avg_jams_resp, 100);

    printf("\n--- Estatisticas Detalhadas ---\n");
    printf("RED : Aceitou %.2f/%d tarefas (media). Tempo Medio: %.2f ms\n", avg_red_acc, total_tasks, avg_red_resp);
    printf("JAMS: Aceitou %.2f/%d tarefas (media). Tempo Medio: %.2f ms\n", avg_jams_acc, total_tasks, avg_jams_resp);

    // salvar CSV
    FILE *f = fopen("logs/dados_comparativos.csv", "w");
    if (f) {
        fprintf(f, "Algoritmo,TarefasAceitasMedia,TempoRespostaMedia_ms\n");
        fprintf(f, "RED,%.2f,%.2f\n", avg_red_acc, avg_red_resp);
        fprintf(f, "JAMS,%.2f,%.2f\n", avg_jams_acc, avg_jams_resp);
        fclose(f);
        printf("\n[INFO] Dados exportados para 'logs/dados_comparativos.csv'\n");

    } else {
        fprintf(stderr, "Erro ao criar arquivo de saida CSV\n");
    }

    free(tasks);
    return 0;
}
