#include <stdio.h>
#include <stdlib.h>
#include "red.h"
#include "parser.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: ./red_sim trace.csv\n");
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("Erro abrindo trace");
        return 1;
    }

    char line[256];
    job_t job;
    red_state_t state;
    red_init(&state);

    while (fgets(line, sizeof(line), f)) {
        if (!parse_job(line, &job))
            continue;

        state.now = job.release_time;

        if (red_accept(&job, &state)) {
            red_execute(&job, &state);
        }
    }

    fclose(f);

    printf("Jobs executados: %lu\n", state.executed_jobs);
    printf("Deadlines perdidos: %lu\n", state.missed_deadlines);

    return 0;
}

