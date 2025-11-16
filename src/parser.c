#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int parse_job(const char *line, job_t *j) {
    return sscanf(
        line, "%d,%lu,%lu,%lu",
        &j->job_id,
        &j->release_time,
        &j->exec_time,
        &j->abs_deadline
    ) == 4;
}

