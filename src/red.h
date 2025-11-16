#ifndef RED_H
#define RED_H

#include <stdint.h>

typedef struct {
    int job_id;
    uint64_t release_time;
    uint64_t exec_time;
    uint64_t abs_deadline;
} job_t;

typedef struct {
    uint64_t now;
    uint64_t missed_deadlines;
    uint64_t executed_jobs;
} red_state_t;

void red_init(red_state_t *s);
int red_accept(const job_t *j, const red_state_t *s);
void red_execute(job_t *j, red_state_t *s);

#endif

