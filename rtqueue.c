#define _GNU_SOURCE
#include <unistd.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "distrib.h"
#include "ts.h"
#include "estim.h"
#include "dw_debug.h"
#include "dl_util.h"

/* Data structure representing a job submitted to the JAMS system */
typedef struct {
  unsigned int C_us;
  struct timespec deadline_ts;
  struct timespec sent;
  long elapsed_us;
} job_t;

#define MAX_SIZE 128

/* Data structure representing a globally shared JAMS queue */
typedef struct {
  job_t *elems[MAX_SIZE];
  int head; // head of the queue
  int tail; // tail of the queue
  int size; // size of the queue
  pthread_mutex_t mtx;   // mutex for concurrent access to the shared JAMS queue
  pthread_cond_t empty;  // condvar where a reader blocks on pull(), and gets notified by another thread on push()

  /* Note: the blocking logic on push() was NOT needed for the JAMS paper runs */
  pthread_cond_t full;   // condvar where a writer blocks on push(), and gets notified by another thread on pull()
} rtqueue_t;

// used when exiting the program
int exiting = 0;

/* The following parameters can be controlled from command-line arguments */

int push_drop_size = MAX_SIZE;
int pop_feasible_jobs = 0;
double prob_dismiss_wcet_us = 0;
double comp_time_perc_us = 0;
double estim_perc = 0.0;
double u_tot = NAN;
int affinity_cpu = -1;
int fine_tune = 0;
int measure_overheads = 0;
unsigned long dismiss_point_us = 0;

unsigned long dl_runtime_us = 0;
unsigned long dl_period_us = 0;

#define MAX_NUM_REQS (1024*1024)
job_t jobs[MAX_NUM_REQS];

// A dummy job used to cause workers to exit
job_t dummy = { 0, { 0, 0 }, { 0, 0 }, 0 };

/* Data structure representing the information passed to each JAMS
   worker thread when created */
typedef struct {
  pthread_t pthr;
  pid_t tid;
} thread_info_t;

rtqueue_t q;

int num_reqs = 10;
int num_child = 1;

#define MAX_NUM_CHILD 128
thread_info_t child[MAX_NUM_CHILD];

unsigned long push_elapsed_ns[MAX_NUM_REQS];
unsigned long pop_elapsed_ns[MAX_NUM_CHILD][MAX_NUM_REQS];
int pop_elapsed_num[MAX_NUM_CHILD];

/* Initialization function, to be called before any other operation on
   a rtqueue_t instance */
void rtq_init(rtqueue_t *pq) {
  pq->head = pq->tail = pq->size = 0;
  pthread_mutex_init(&pq->mtx, NULL);
  pthread_cond_init(&pq->empty, NULL);
  pthread_cond_init(&pq->full, NULL);
}

/* Cleanup function, to be called once you're done with a rtqueue_t
   instance */
void rtq_cleanup(rtqueue_t *pq) {
  pthread_mutex_destroy(&pq->mtx);
  pthread_cond_destroy(&pq->empty);
  pthread_cond_destroy(&pq->full);
}

// Variaveis global do RED
double red_min_th = 20;   // exemplo
double red_max_th = 80;   // exemplo
double red_max_p  = 0.1;  // probabilidade máxima 10%

double red_avg = 0.0;     // average queue length
double red_w_q = 0.002;   // weight do moving average (padrão RED)


/* Push the specified job into the shared JAMS queue */
int rtq_push(rtqueue_t *pq, job_t *p_elem) {
    dw_log("pushing job %ld (%p)\n", p_elem - jobs, (void*)p_elem);
    int rv = 0;
    pthread_mutex_lock(&pq->mtx);
    if (pq->size == MAX_SIZE)
        goto unlock;

    /* --- RED CLASSIC DROP POLICY -------------------------------- */

      /* só ativa RED quando a fila entra na zona "congestionada"
       (equivalente ao push_drop_size como base de ativação) */
    if (pq->size > push_drop_size) {

        /* Atualiza a média móvel exponencial */
        red_avg = (1.0 - red_w_q) * red_avg + red_w_q * pq->size;

        double p_drop = 0.0;

        if (red_avg < red_min_th) {
            /* Não descarta */
            p_drop = 0.0;

        } else if (red_avg >= red_max_th) {
            /* Descarte total */
            p_drop = 1.0;

        } else {
            /* Descarta com probabilidade crescente (linear) */
            p_drop = red_max_p *
                    (red_avg - red_min_th) /
                    (red_max_th - red_min_th);
        }

        /* Decide descartar conforme p_drop */
        if (p_drop > 0.0) {
            double r = (double)rand() / (double)RAND_MAX;

            if (r < p_drop) {
                dw_log("[RED] drop job p=%f avg=%f size=%d\n",
                       p_drop, red_avg, pq->size);
                goto unlock;   /* descarta o push */
            }
        }
    }

    /* --- Se não descartou, push normal ------------------------------------- */

    pq->elems[pq->head] = p_elem;
    pq->head = (pq->head + 1) % MAX_SIZE;
    pq->size++;
    pthread_cond_broadcast(&pq->empty);
    rv = 1;

unlock:
    pthread_mutex_unlock(&pq->mtx);
    
    return rv;
}

job_t *rtq_peekn_nosync(rtqueue_t *pq, int n) {
  if (pq->size <= n)
    return NULL;
  return pq->elems[(pq->tail + n) % MAX_SIZE];
}

job_t *rtq_pop_nosync(rtqueue_t *pq) {
  if (pq->size == 0)
    return NULL;
  job_t *p_elem = pq->elems[pq->tail];
  pq->tail = (pq->tail + 1) % MAX_SIZE;
  pq->size--;

  return p_elem;
}

job_t *rtq_popn_nosync(rtqueue_t *pq, int n) {
  if (pq->size <= n)
    return NULL;
  job_t *p_elem = pq->elems[(pq->tail + n) % MAX_SIZE];
  for (int i = pq->tail + n; i > pq->tail; i--)
    pq->elems[(i-1) % MAX_SIZE] = pq->elems[i % MAX_SIZE];
  pq->tail = (pq->tail + 1) % MAX_SIZE;
  pq->size--;

  return p_elem;
}

long lmax(long a, long b) {
  return a > b ? a : b;
}

long lceil(long a, long b) {
  return (a + b - 1) / b;
}

long budget_to_deadline(long runtime_left_ns, long time_to_dline_ns, long time_to_job_dline_ns) {
  if (time_to_job_dline_ns <= time_to_dline_ns)
    return lmax(0, runtime_left_ns - lmax(0, time_to_dline_ns * u_tot - time_to_job_dline_ns));
  else {
    int periods = (time_to_job_dline_ns - time_to_dline_ns) / (dl_period_us * 1000l);
    long left_ns = (time_to_job_dline_ns - time_to_dline_ns) % (dl_period_us * 1000l);
    return runtime_left_ns + periods * dl_runtime_us * 1000l + lmax(0, dl_runtime_us * 1000l - lmax(0, dl_period_us * 1000l * u_tot - left_ns));
  }
}

job_t *rtq_pop_dl_nosync(rtqueue_t *pq) {
  job_t *p_job = NULL;
  if (pq->size == 0)
    goto out;

  long runtime_left_ns, abs_deadline_ns;
  if (dl_runtime_us > 0) {
    dl_params_get(gettid(), &runtime_left_ns, &abs_deadline_ns);
    abs_deadline_ns = deadline_to_monotonic(abs_deadline_ns);
  }
  struct timespec now_ts;
  clock_gettime(CLOCK_MONOTONIC, &now_ts);

  for (int n = 0; n < pq->size; n++) {
    dw_log("peeking at elem n=%d, size=%d\n", n, pq->size);
    job_t *p_elem = rtq_peekn_nosync(pq, n);
    assert(p_elem != NULL);
    // if this job deadline has already passed, dismiss it
    long slack_ns = ts_sub_ns(&p_elem->deadline_ts, &now_ts);
    dw_log("job %d (%p) slack_ns to deadline %ld\n", (int)(p_elem - jobs), (void*)p_elem, slack_ns);
    if (slack_ns < 0) {
      dw_log("dropping late job %d (%p)\n", (int)(p_elem - jobs), (void*)p_elem);
      check(rtq_popn_nosync(pq, n) == p_elem);
      n--;
      continue;
    }
    long C_ns = comp_time_perc_us * 1000l;
    int accept_job = 0;
    if (prob_dismiss_wcet_us == 0) {
      /* old policy: dismiss deterministically */
      long finish_time_ns;
      if (C_ns < runtime_left_ns) {
        finish_time_ns = abs_deadline_ns; // Eq. (5) in the draft
        finish_time_ns -= (1.0 - u_tot) * slack_ns;
        if (fine_tune)
          finish_time_ns -= runtime_left_ns - C_ns; // ?!?
      } else {
        long dl_runtime_ns = dl_runtime_us * 1000;
        long dl_period_ns = dl_period_us * 1000;
        finish_time_ns = abs_deadline_ns
          + lceil(C_ns - runtime_left_ns, dl_runtime_ns) * dl_period_ns;
        finish_time_ns -= (1.0 - u_tot) * dl_period_us * 1000;
        if (fine_tune)
          finish_time_ns -= dl_runtime_ns - (C_ns - runtime_left_ns) % dl_runtime_ns;
      }
      dw_log("job %d (%p) C_ns %ld abs_deadline_ns %ld finish_time_ns %ld\n", (int)(p_elem - jobs), (void*)p_elem, C_ns, abs_deadline_ns, finish_time_ns);
      // Eq. (1) in the draft
      if (ts_to_ns(p_elem->deadline_ts) - finish_time_ns >= 0)
        accept_job = 1;
    } else {
      /* new policy: dismiss with probability increasing linearly with the usable budget till job deadline going from the desired percentile to the WCET */
      long avail_ns = budget_to_deadline(runtime_left_ns, abs_deadline_ns - ts_to_ns(now_ts), slack_ns);
      dw_log("job %d (%p) perc_ns %ld avail_ns %ld wcet_ns %ld\n", (int)(p_elem - jobs), (void*)p_elem, C_ns, avail_ns, (long)(prob_dismiss_wcet_us * 1000l));
      if (avail_ns >= prob_dismiss_wcet_us * 1000l)
        /* WCET guaranteed: accept job for process */
        accept_job = 1;
      else if (avail_ns >= C_ns) {
        /* avail_ns between perc and wcet, apply probabilistic dismissal */
        float prob = (avail_ns - C_ns) / (float)(prob_dismiss_wcet_us * 1000.0 - C_ns);
        unsigned long r = rand();
        dw_log("prob %f rand %f\n", prob, (float)r / (float)RAND_MAX);
        if ((float)r / (float)RAND_MAX < prob)
          accept_job = 1;
      }
    }
    if (accept_job) {
      // accepting job for processing
      dw_log("extracting job %d (%p)\n", (int)(p_elem - jobs), (void*)p_elem);
      check(rtq_popn_nosync(pq, n) == p_elem);
      p_job = p_elem;
      break;
    } else {
      /* if not processed, the job remains on the queue, might be chosen by
         other workers, or even by us later (more unlikely to take it than now) */
      dw_log("leaving job %d (%p) in queue\n", (int)(p_elem - jobs), (void*)p_elem);
    }
  }

 out:
  return p_job;
}

// returns true if job finished, false if dismissed
int consume_us(job_t *p_job) {
  struct timespec ts_beg, ts_end;
  unsigned long elapsed_us;
  unsigned long curr_resp_time_us;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_beg);
  do {
    struct timespec ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    curr_resp_time_us = (ts_now.tv_sec - p_job->sent.tv_sec) * 1000000 + (ts_now.tv_nsec - p_job->sent.tv_nsec) / 1000;

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);
    elapsed_us = (ts_end.tv_sec - ts_beg.tv_sec) * 1000000 + (ts_end.tv_nsec - ts_beg.tv_nsec) / 1000;
  } while (elapsed_us < p_job->C_us && (dismiss_point_us == 0 || curr_resp_time_us <= dismiss_point_us));
  return elapsed_us >= p_job->C_us;
}

void sched_dl_params_overhead() {
  pid_t tid = gettid();
  long runtime_ns, deadline_ns;
  struct timespec ts1, ts2;
  check(clock_gettime(CLOCK_MONOTONIC, &ts1) == 0);
  dl_params_get(tid, &runtime_ns, &deadline_ns);
  check(clock_gettime(CLOCK_MONOTONIC, &ts2) == 0);
  long overhead_ns = ts_sub_ns(&ts2, &ts1);
  printf("get_sched_dl_params() overhead_ns: %ld = %ld,%ld - %ld,%ld\n", overhead_ns, ts2.tv_sec, ts2.tv_nsec, ts1.tv_sec, ts1.tv_nsec);
}

void sched_set_deadline(unsigned long runtime_us, unsigned long deadline_us, unsigned long period_us) {
  struct sched_attr attr = {
    .size = sizeof(struct sched_attr),
    .sched_policy = SCHED_DEADLINE,
    .sched_flags = 0,
    .sched_runtime  = runtime_us * 1000,
    .sched_deadline = deadline_us * 1000,
    .sched_period   = period_us * 1000
  };
  if (sched_setattr(0, &attr, 0) < 0) {
    perror("setattr() failed");
  }
}

/* Pull a job out of the JAMS shared queue */
job_t *rtq_pop(rtqueue_t *pq) {
  job_t *p_elem = NULL;
  pthread_mutex_lock(&pq->mtx);

  while (!exiting) {
    if (pq->size > 0) {
      if (pop_feasible_jobs)
        p_elem = rtq_pop_dl_nosync(pq);
      else
        p_elem = rtq_pop_nosync(pq);
    }

    if (p_elem != NULL)
      break;

    pthread_cond_wait(&pq->empty, &pq->mtx);
  }

  pthread_cond_signal(&pq->full);
  pthread_mutex_unlock(&pq->mtx);

  return p_elem;
}

void rtq_wait_until_empty(rtqueue_t *pq) {
  while (pq->size > 0) {
    dw_log("wait_until_empty(): size=%d\n", pq->size);
    pthread_cond_broadcast(&pq->empty);
    usleep(100000);
  }
}

pthread_barrier_t barrier;

/* Set the current thread affinity to the specified single CPU (0-based) */
void set_affinity(int cpu) {
  cpu_set_t cpus;
  CPU_ZERO(&cpus);
  CPU_SET(cpu, &cpus);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpus), &cpus) != 0) {
    perror("pthread_setaffinity_np() failed");
    exit(1);
  }
}

/* Main JAMS worker thread entry function */
void *worker(void *arg) {
  thread_info_t *pinfo = (thread_info_t *) arg;
  int thread_id = pinfo - child; // just 0, 1, ...; not a Linux TID
  pinfo->tid = gettid();

  // workers are pinned to affinity_cpu + 1, + 2, etc...
  if (affinity_cpu != -1)
    set_affinity(affinity_cpu + thread_id + 1);

  if (dl_runtime_us > 0) {
    sched_set_deadline(dl_runtime_us, dl_period_us, dl_period_us);
    sched_dl_params_overhead();
    dl_sync(pinfo->tid, dl_period_us);
  }

  estim est;
  if (estim_perc > 0)
    estim_init(&est, estim_perc);

  pthread_barrier_wait(&barrier);

  for (int i = 0; !exiting; i++) {
    struct timespec ts_beg;
    if (measure_overheads)
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_beg);

    job_t *p_job = rtq_pop(&q);

    if (measure_overheads) {
      struct timespec ts_end;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);
      assert(pop_elapsed_num[thread_id] < MAX_NUM_REQS);
      pop_elapsed_ns[thread_id][pop_elapsed_num[thread_id]++] = (ts_end.tv_sec - ts_beg.tv_sec) * 1000000000l + (ts_end.tv_nsec - ts_beg.tv_nsec);
    }

    if (p_job == NULL || p_job == &dummy)
      break;

    if (!consume_us(p_job)) {
      // technically unneeded, just remarking this will job be counted as dismissed
      p_job->elapsed_us = 0;
      continue;
    }

    if (estim_perc > 0) {
      estim_add_sample(&est, p_job->C_us);
      if (i > 0) {
        comp_time_perc_us = estim_get_quantile(&est);
        dw_log("estimated percentile: %g\n", comp_time_perc_us);
      }
    }

    struct timespec ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    p_job->elapsed_us = (ts_end.tv_sec - p_job->sent.tv_sec) * 1000000 + (ts_end.tv_nsec - p_job->sent.tv_nsec) / 1000;
    assert(p_job->elapsed_us >= p_job->C_us - 1); // tolerate 1us lost
  }
  return 0;
}

pd_spec_t pd_comp_time_us;
pd_spec_t pd_period_us;
pd_spec_t pd_deadline_us;
unsigned long seed;
dl_params_type_t dlpar_type = DL_PARAMS_AUTO;

int main(int argc, char *argv[]) {
  pd_comp_time_us = pd_build_fixed(1000);
  pd_period_us = pd_build_fixed(10000);
  pd_deadline_us = pd_build_fixed(10000);
  seed = time(NULL);

  argc--;  argv++;
  while (argc > 0) {
    if (strcmp(*argv, "-h") == 0 || strcmp(*argv, "--help") == 0) {
      printf("Usage: rtqueue [-h|--help] [-t|--threads num_threads] [-a|--set-affinity cpu] [-j|--jobs num_jobs] [-c|--comp-time val|distrib] [-p|--period val|distrib] [-d|--deadline val|distrib] [-dr|--dl-runtime us] [-dp|--dl-period us] [-s|--seed val] [-ft|--fine-tune] [-pds|--push-drop-size queue_size] [-%%|--percentile perc_us] [-pd-wcet|--prob-dismiss-wcet us] [-ep|--estimate-percentile val] [-u|--utilization per_cpu_val] [-dlp|--dl-params auto|getattr|kmod|proc] [-o|--overheads] [--dismiss-point us]\n");
      exit(EXIT_SUCCESS);
    } else if (strcmp(*argv, "-t") == 0 || strcmp(*argv, "--threads") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf(*argv, "%d", &num_child) == 1);
      check(num_child > 0 && num_child < MAX_NUM_CHILD);
    } else if (strcmp(*argv, "-a") == 0 || strcmp(*argv, "--set-affinity") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf(*argv, "%d", &affinity_cpu) == 1);
    } else if (strcmp(*argv, "-j") == 0 || strcmp(*argv, "--jobs") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf(*argv, "%d", &num_reqs) == 1);
      check(num_reqs > 0 && num_reqs < MAX_NUM_REQS);
    } else if (strcmp(*argv, "-dr") == 0 || strcmp(*argv, "--dl-runtime") == 0) {
      argc--;  argv++;
      check(argc > 0);
      double value;
      check(sscanf_unit(*argv, "%lf", &value, 1) == 1);
      dl_runtime_us = value;
    } else if (strcmp(*argv, "-dp") == 0 || strcmp(*argv, "--dl-period") == 0) {
      argc--;  argv++;
      check(argc > 0);
      double value;
      check(sscanf_unit(*argv, "%lf", &value, 1) == 1);
      dl_period_us = value;
    } else if (strcmp(*argv, "-c") == 0 || strcmp(*argv, "--comp-time") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(pd_parse_time(&pd_comp_time_us, *argv));
    } else if (strcmp(*argv, "-p") == 0 || strcmp(*argv, "--period") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(pd_parse_time(&pd_period_us, *argv));
    } else if (strcmp(*argv, "-d") == 0 || strcmp(*argv, "--deadline") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(pd_parse_time(&pd_deadline_us, *argv));
    } else if (strcmp(*argv, "-pds") == 0 || strcmp(*argv, "--push-drop-size") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf(*argv, "%d", &push_drop_size) == 1);
    } else if (strcmp(*argv, "-%") == 0 || strcmp(*argv, "--percentile") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf_unit(*argv, "%lf", &comp_time_perc_us, 1) == 1);
      if (comp_time_perc_us > 0)
        pop_feasible_jobs = 1;
    } else if (strcmp(*argv, "-pd-wcet") == 0 || strcmp(*argv, "--prob-dismiss-wcet") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf_unit(*argv, "%lf", &prob_dismiss_wcet_us, 1) == 1);
    } else if (strcmp(*argv, "-ep") == 0 || strcmp(*argv, "--estimate-percentile") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf(*argv, "%lf", &estim_perc) == 1);
    } else if (strcmp(*argv, "-u") == 0 || strcmp(*argv, "--utilization") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf(*argv, "%lf", &u_tot) == 1);
      check(u_tot > 0.0 && u_tot <= 1.0, "The total per-cpu utilization must be in the range ]0, 1.0]\n");
    } else if (strcmp(*argv, "-ft") == 0 || strcmp(*argv, "--fine-tune") == 0) {
      fine_tune = 1;
    } else if (strcmp(*argv, "-o") == 0 || strcmp(*argv, "--overheads") == 0) {
      measure_overheads = 1;
    } else if (strcmp(*argv, "-s") == 0 || strcmp(*argv, "--seed") == 0) {
      argc--;  argv++;
      check(argc > 0);
      check(sscanf(*argv, "%lu", &seed) == 1);
    } else if (strcmp(*argv, "-dlp") == 0 || strcmp(*argv, "--dl-params") == 0) {
      argc--;  argv++;
      check(argc > 0);
      if (strcmp(*argv, "auto") == 0)
        dlpar_type = DL_PARAMS_AUTO;
      else if (strcmp(*argv, "getattr") == 0)
        dlpar_type = DL_PARAMS_GETATTR;
      else if (strcmp(*argv, "kmod") == 0)
        dlpar_type = DL_PARAMS_KMOD;
      else if (strcmp(*argv, "proc") == 0)
        dlpar_type = DL_PARAMS_PROC;
      else {
        fprintf(stderr, "Wrong argument to -dlp|--dl-params option: %s\n", argv[0]);
        exit(1);
      }
    } else if (strcmp(*argv, "--dismiss-point") == 0) {
      argc--;  argv++;
      check(argc > 0);
      double value;
      check(sscanf_unit(*argv, "%lf", &value, 1) == 1);
      dismiss_point_us = value;
    } else {
      fprintf(stderr, "Unknown option: %s\n", *argv);
      exit(1);
    }
    argc--;  argv++;
  }

  check(dl_params_init(dlpar_type) == 0, "dl_params_init() failed!");

  if (isnan(u_tot))
    u_tot = dl_runtime_us / (double)dl_period_us;

  printf("Options:\n");
  printf("   threads: %d\n", num_child);
  printf("  affinity: %d\n", affinity_cpu);
  printf("      jobs: %d\n", num_reqs);
  printf(" comp-time: %s us\n", pd_str(&pd_comp_time_us));
  printf("    period: %s us\n", pd_str(&pd_period_us));
  printf("  deadline: %s us\n", pd_str(&pd_deadline_us));
  printf("dl-runtime: %lu us\n", dl_runtime_us);
  printf(" dl-period: %lu us\n", dl_period_us);
  printf("       pds: %d\n", push_drop_size);
  printf("  pop-feas: %d\n", pop_feasible_jobs);
  printf("   perc-us: %g us\n", comp_time_perc_us);
  printf("   pd-wcet: %g us\n", prob_dismiss_wcet_us);
  printf("estim-perc: %g%%\n", estim_perc*100);
  printf("     u-tot: %g\n", u_tot);
  printf(" fine_tune: %d\n", fine_tune);
  printf(" overheads: %d\n", measure_overheads);
  printf("dismiss p.: %lu us\n", dismiss_point_us);
  printf("      seed: %lu\n", seed);
  printf("  dlparams: %s\n", dl_params_str());

  check((dl_runtime_us > 0 && dl_runtime_us < dl_period_us)
         || (dl_runtime_us == 0 && dl_period_us == 0));

  check(pop_feasible_jobs == 0 || dl_runtime_us > 0);

  check(prob_dismiss_wcet_us == 0 || prob_dismiss_wcet_us >= comp_time_perc_us);

  rtq_init(&q);
  pd_init(seed);

  if (measure_overheads)
    for (int i = 0; i < MAX_NUM_CHILD; i++)
      pop_elapsed_num[i] = 0;

  // parent pinned on affinity_cpu, workers on following ones
  if (affinity_cpu != -1)
    set_affinity(affinity_cpu);

  pthread_barrier_init(&barrier, NULL, num_child + 1);

  for (int i = 0; i < num_child; i++) {
    pthread_create(&child[i].pthr, NULL, &worker, &child[i]);
  }

  pthread_barrier_wait(&barrier);

  struct timespec ts_next;
  clock_gettime(CLOCK_MONOTONIC, &ts_next);
  for (int j = 0; j < num_reqs; j++) {
    clock_gettime(CLOCK_MONOTONIC, &jobs[j].sent);
    jobs[j].C_us = ceil(pd_sample(&pd_comp_time_us));
    //printf("C_us=%u\n", jobs[j].C_us);
    jobs[j].deadline_ts = jobs[j].sent;
    ts_add_us(&jobs[j].deadline_ts, pd_sample(&pd_deadline_us));

    struct timespec ts_beg;
    if (measure_overheads)
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_beg);

    if (rtq_push(&q, &jobs[j]))
      jobs[j].elapsed_us = 0;
    else
      jobs[j].elapsed_us = -1;

    if (measure_overheads) {
      struct timespec ts_end;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts_end);
      push_elapsed_ns[j] = (ts_end.tv_sec - ts_beg.tv_sec) * 1000000000l + (ts_end.tv_nsec - ts_beg.tv_nsec);
    }

    if (j == 0 || (j-1) * 10 / num_reqs < j * 10 / num_reqs) {
      fprintf(stderr, "%d%%...", j*100/num_reqs);
      fflush(stderr);
    }
    ts_add_us(&ts_next, pd_sample(&pd_period_us));
    check(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts_next, NULL) == 0);
  }
  fprintf(stderr, "\n");

  printf("Waiting for empty queue...\n");
  rtq_wait_until_empty(&q);

  printf("Terminating and joining workers...\n");

  exiting = 1;

  // cause exit of worker threads as they pop &dummy out of q
  for (int i = 0; i < num_child; i++)
    // repeat in case push doesn't succeed (full queue or dismissed job)
    while (!rtq_push(&q, &dummy))
      usleep(1000);

  for (int i = 0; i < num_child; i++) {
    pthread_join(child[i].pthr, NULL);
  }

  rtq_cleanup(&q);

  dl_params_cleanup();

  long ref_us = ts_to_us(jobs[0].sent);
  for (int j = 0; j < num_reqs; j++)
    printf("request %d sent_us %lu C_us %u elapsed_us %d\n", j, ts_to_us(jobs[j].sent) - ref_us, jobs[j].C_us, (int)jobs[j].elapsed_us);

  if (measure_overheads) {
    for (int j = 0; j < num_reqs; j++)
      printf("overheads: request %d push_elapsed_ns: %lu\n", j, push_elapsed_ns[j]);
    for (int i = 0; i < num_child; i++)
      for (int j = 0; j < pop_elapsed_num[i]; j++)
        printf("overheads: thread %d pop_elapsed_ns: %lu\n", i, pop_elapsed_ns[i][j]);
  }
}
