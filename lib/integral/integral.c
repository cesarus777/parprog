#include "flt_type.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



enum { BASE = 10 };
enum { FORMAT_LEN = 24 };
enum { N_STEPS = 100 };



typedef struct {
  struct {
    flt_type start;
    flt_type end;
  } * ranges;

  int n_range;

  pthread_mutex_t mutex;
} points_stack_t;



flt_type f(const flt_type x) { return flt_sin(1 / x); }

void *job(void *arg);

int get_n_jobs(int argc, const char *argv[]);



int main(int argc, const char *argv[]) {
  int n_jobs = get_n_jobs(argc, argv);

  if (n_jobs < 1) {
    fprintf(stderr, "%s: fatal error: num of jobs must be a natural number\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  const flt_type start = 1e-3;
  const flt_type end   = 1e0;

  const flt_type step = (end - start) / (flt_type) N_STEPS;

  points_stack_t stack;

  stack.n_range = 0;
  stack.ranges  = malloc(N_STEPS * sizeof(*stack.ranges));
  assert(stack.ranges);

  for (int i = 0; i < N_STEPS; i++) {
    stack.ranges[i].start = start + step * i;

    if (i == (N_STEPS - 1)) {
      stack.ranges[i].end = end;
    } else {
      stack.ranges[i].end = start + step * (i + 1);
    }
  }

  pthread_mutex_init(&stack.mutex, NULL);

  pthread_t *threads = malloc(n_jobs * sizeof(pthread_t));
  for (int i = 0; i < n_jobs; i++) {
    pthread_create(threads + i, NULL, &job, (void *) &stack);
  }

  flt_type int_res = 0.0;

  for (int i = 0; i < n_jobs; i++) {
    flt_type *thread_res;
    pthread_join(threads[i], (void **) &thread_res);
    assert(thread_res);
    int_res += *thread_res;
    free(thread_res);
  }

  pthread_mutex_destroy(&stack.mutex);

  char format[FORMAT_LEN];
  sprintf(format, "%%.%d" FLT_PRINT_FORMAT "\n", FLT_TYPE_DIG);
  printf(format, int_res);
}



flt_type f_2der(flt_type x, flt_type (*f)(flt_type)) {
  // TODO: remove this hack and calc it generally
  f(FLT_ZERO);
  return (2 * x * flt_cos(FLT_ONE / x) - flt_sin(FLT_ONE / x)) /
         ((x * x) * (x * x));
}



flt_type f_2der_max(flt_type start, flt_type end, flt_type (*f)(flt_type)) {
  flt_type       max  = flt_abs(f_2der(start, f));
  const flt_type step = 1e-6;
  for (int i = 1; i < (end - start) / step; i++) {
    flt_type val = flt_abs(f_2der(start + step * i, f));
    if (max < val) {
      max = val;
    } else {
      return max;
    }
  }

  return max;
}



flt_type get_adaptive_step(flt_type start, flt_type end,
                           flt_type (*f)(flt_type)) {
  const flt_type range_len = end - start;

  const flt_type adapt_size =
      flt_sqrt(1e-3 * 12 / (f_2der_max(start, end, f)) * (range_len));

  const flt_type step = 1e6;

  return (adapt_size < range_len / step) ? adapt_size : range_len / step;
}



flt_type integrate(flt_type start, flt_type end, flt_type (*f)(flt_type)) {
  const flt_type step    = get_adaptive_step(start, end, f);
  flt_type       int_sum = FLT_ZERO;

  for (int i = 1; start + (i + 1) * step < end; i++) {
    int_sum += f(start + i * step);
  }

  // NOLINTNEXTLINE: formula for numerical integration
  int_sum += f(start) / 2.0 + f(end - step) / 2.0;
  int_sum *= step;

  return int_sum;
}



void *job(void *arg) {
  assert(arg);

  flt_type *thread_res = malloc(sizeof(flt_type));
  *thread_res          = FLT_ZERO;

  points_stack_t *stack = (points_stack_t *) arg;
  pthread_mutex_lock(&stack->mutex);

  int n_range = stack->n_range++;

  if (n_range >= N_STEPS) {
    pthread_mutex_unlock(&stack->mutex);
    return thread_res;
  }

  pthread_mutex_unlock(&stack->mutex);

  while (n_range < N_STEPS) {
    flt_type start = stack->ranges[n_range].start;
    flt_type end   = stack->ranges[n_range].end;

    *thread_res += integrate(start, end, &f);

    pthread_mutex_lock(&stack->mutex);
    n_range = stack->n_range++;
    pthread_mutex_unlock(&stack->mutex);
  }

  return thread_res;
}


int get_n_jobs(const int argc, const char *argv[]) {
  if ((argc == 2) && (strncmp(argv[1], "-j=", 3) == 0)) {
    char *endptr;
    errno = 0;

    long int n_jobs = strtol(argv[1] + 3, &endptr, BASE);
    if ((*endptr != '\0') ||
        (errno == ERANGE && (n_jobs == LONG_MAX || n_jobs == LONG_MIN)) ||
        (errno != 0 && n_jobs == 0)) {
      return -1;
    }

    return n_jobs;
  }

  if ((argc == 3) && (strcmp(argv[1], "-j") == 0)) {
    char *endptr;
    errno = 0;

    long int n_jobs = strtol(argv[2], &endptr, BASE);
    if ((*endptr != '\0') ||
        (errno == ERANGE && (n_jobs == LONG_MAX || n_jobs == LONG_MIN)) ||
        (errno != 0 && n_jobs == 0)) {
      return -1;
    }

    return n_jobs;
  }

  fprintf(stderr, "%s: fatal error: specify num of jobs via '-j n' or '-j=n' option\n",
          argv[0]);
  exit(EXIT_FAILURE);
  return -1;
}

