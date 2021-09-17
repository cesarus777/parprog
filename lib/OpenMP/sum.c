#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



enum { BASE = 10 };
enum { MSG_SIZE = 64, NUM_STR_SIZE = 24 };
enum { VALID_NUMBER, BAD_ARGC, BAD_NUMBER, TOO_BIG_NUMBER };

int get_n(long *n, int argc, char *argv[]);
long get_max_supported();

long max_supported = 0;



int main(int argc, char *argv[]) {
  max_supported = get_max_supported();
  long N   = 0;
  int  res = get_n(&N, argc, argv);
  if (res != VALID_NUMBER) {
    const char *error_message = NULL;
    switch (res) {
    case BAD_NUMBER:
      error_message = "second argument number must be a natural number";
      break;
    case TOO_BIG_NUMBER: {
      char buf[MSG_SIZE] = "only support numbers up to ";
      char max_num[NUM_STR_SIZE];
      snprintf(max_num, NUM_STR_SIZE - 1, "%ld", max_supported);
      error_message = strcat(buf, max_num);
      break;
    }
    case BAD_ARGC:
      error_message = "one argument expected";
      break;
    default:
      error_message = "unknown error";
      break;
    }

    fprintf(stderr, "%s: fatal error: %s\n", argv[0], error_message);
    exit(EXIT_FAILURE);
  }

  unsigned long long sum = 0;

#pragma omp parallel for reduction(+:sum) //schedule(static, N / omp_get_num_threads())
  for (long i = 1; i <= N; i++) {
    sum += i;
  }

  printf("%llu\n", sum);
}



int get_n(long *n, int argc, char *argv[]) {
  if (argc == 2) {
    char *endptr;
    errno = 0;

    long num = strtol(argv[1], &endptr, BASE);
    if (((errno == ERANGE) && (num == LONG_MAX)) || (num > max_supported)) {
      return TOO_BIG_NUMBER;
    }
    if ((*endptr != '\0') || (errno != 0) ||
        (num < 1)) { // natural number expected
      return BAD_NUMBER;
    }

    *n = num;
    return VALID_NUMBER;
  }

  return BAD_ARGC;
}

long get_max_supported() {
  unsigned long long max_sum = ULLONG_MAX;

  // calculate max elem using formula for the sum of arithmetic progression:
  // by solving equation (n * (n + 1)) / 2 = max_sum
  // which transforms to n ^ 2 + n - 2 * max_sum = 0
  double D = 1 + 4.0 * 2.0 * max_sum; // NOLINT: discriminant formula
  long max_elem = (-1 + sqrt(D)) / 2.0; // NOLINT: root formula
  return max_elem;
}

