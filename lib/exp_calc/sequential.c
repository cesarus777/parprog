#include "flt_type.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



enum { BASE = 10 };
enum { FORMAT_LEN = 24 };



int count_max_pow(int n_digits) {
  flt_type c = -flt_log(2) + n_digits * flt_log(BASE);

  const flt_type k_start     = 1.0;
  const flt_type knext_start = 2.0;
  const flt_type epsilon     = 0.5;

  flt_type k     = k_start;
  flt_type knext = knext_start;
  while ((knext - k) > epsilon) {
    k     = knext;
    knext = (k + c) / flt_log(k);
  }
  return (int) k;
}



flt_type calc_exp(int n_digits) {
  flt_type sum     = FLT_ONE;
  uint64_t fact_i  = 1;
  int      max_pow = count_max_pow(n_digits);
  for (int i = 0; i < max_pow; i++) {
    fact_i *= i + 1;
    sum += FLT_ONE / (flt_type) fact_i;
  }

  return sum;
}



int get_n_digits(const char *str) {
  if (str == NULL) {
    return FLT_TYPE_DIG;
  }

  char *endptr;
  errno  = 0;
  long n = strtol(str, &endptr, BASE);
  if (((errno == ERANGE) && (n == LONG_MAX || n == LONG_MIN)) || (n < 0) ||
      (*endptr != '\0')) {
    return -1;
  }

  if (n > FLT_TYPE_DIG) {
    n = FLT_TYPE_DIG;
  }

  return n;
}



int main(int argc, char *argv[]) {
  if (argc > 2) {
    fprintf(stderr, "%s: too much arguments\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  int n_digits = FLT_TYPE_DIG;
  if (argc == 2) {
    n_digits = get_n_digits(argv[1]);
  }

  int digits_after_dot = n_digits - 1;
  if (digits_after_dot < 0) {
    fprintf(stderr, "%s: number is not in an appropriate format\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char format[FORMAT_LEN];
  sprintf(format, "e = %%.%d" FLT_PRINT_FORMAT "\n", digits_after_dot);
  printf(format, calc_exp(digits_after_dot));
}

