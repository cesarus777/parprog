#include "flt_type.h"

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>



enum { FORMAT_LEN = 24 };



int count_max_pow(int n_digits) {
  flt_type c = -flt_log(2) + n_digits * flt_log(10);
  flt_type k = 1.0;
  flt_type knext = 2.0;
  while((knext - k) > 0.5) {
    k = knext;
    knext = (k + c) / flt_log(k);
  }
  return (int) k - 0.5;
}



flt_type calc_exp(int n_digits) {
  flt_type sum = FLT_ONE;
  uint64_t fact_i = 1;
  int max_pow = count_max_pow(n_digits);
  for(int i = 0; i < max_pow; i++) {
    fact_i *= i + 1;
    sum += FLT_ONE / (flt_type) fact_i;
  }

  return sum;
}



int get_n_digits(const char *str) {
  if(str == NULL)
    return FLT_TYPE_DIG;
  char *endptr;
  errno = 0;
  long n = strtol(str, &endptr, 10);
  if(((errno == ERANGE) && (n == LONG_MAX || n == LONG_MIN)) || (n < 0) || (*endptr != '\0'))
    return -1;

  if(n > FLT_TYPE_DIG)
    n = FLT_TYPE_DIG;

  return n;
}



int main(int argc, char *argv[]) {
  if(argc > 2) {
    fprintf(stderr, "%s: too much arguments\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  int n_digits = FLT_TYPE_DIG;
  if(argc == 2)
    n_digits = get_n_digits(argv[1]);

  int digits_after_dot = n_digits - 1;
  if(digits_after_dot < 0) {
    fprintf(stderr, "%s: number is not in an appropriate format\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char format[FORMAT_LEN];
#ifdef FLT_TYPE_FLOAT
  sprintf(format, "e = %%.%df\n", digits_after_dot);
#elif FLT_TYPE_DOUBLE
  sprintf(format, "e = %%.%dlf\n", digits_after_dot);
#elif FLT_TYPE_LONG_DOUBLE
  sprintf(format, "e = %%.%dLf\n", digits_after_dot);
#endif
  printf(format, calc_exp(digits_after_dot));
}

