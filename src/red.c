#include "red.h"
#include <stdio.h>
#include <unistd.h>

void red_init(red_state_t *s) {
    s->now = 0;
    s->missed_deadlines = 0;
    s->executed_jobs = 0;
}

int red_accept(const job_t *j, const red_state_t *s) {
    uint64_t finish_time = s->now + j->exec_time;

    if (finish_time > j->abs_deadline)
        return 0;

    return 1;
}

void red_execute(job_t *j, red_state_t *s) {
    s->now += j->exec_time;

    if (s->now > j->abs_deadline)
        s->missed_deadlines++;

    s->executed_jobs++;
}

