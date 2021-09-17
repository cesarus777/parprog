#include "flt_type.h"

#ifdef PARALLEL
  #include "mpi/mpi.h"
  #include "mpi_error.h"
#endif

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



//#undef NDEBUG

#ifdef PARALLEL

enum {
  PREV_FACT_TAG,
  PREV_SUM_TAG,
};

enum { ROOT_RANK = 0 };

#endif

enum { BASE = 10 };
enum { FORMAT_LEN = 24 };



int log10i(int x) { return (int) (flt_log10(x) + 1 / (flt_type) 2); }

// using Stirling's approximation to find the least n: [1 / (n + 1)!] < [0.5 *
// 10^(n_digits)]
int count_max_elem(int n_digits) {
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
  return (int) (k - epsilon);
}

flt_type calc_exp(int n_digits) {
  flt_type sum      = FLT_ZERO;
  int      max_elem = count_max_elem(n_digits);

#ifndef PARALLEL

  sum             = FLT_ONE;
  uint64_t fact_i = 1;
  for (int i = 0; i < max_elem; i++) {
    fact_i *= i + 1;
    sum += FLT_ONE / (flt_type) fact_i;
  }

#else

  int initialized = 0;
  int finalized   = 1;
  TRY_MPI(MPI_Initialized(&initialized));
  TRY_MPI(MPI_Finalized(&finalized));

  if (!initialized || finalized) {
    return -1;
  }

  int size = -1;
  int rank = -1;

  TRY_MPI(myMPI_get_rank_and_size(&rank, &size));

  if (rank >= max_elem) {
    TRY_MPI(MPI_Finalize());
  #ifndef NDEBUG
    fprintf(stderr, "rank %d finalized\n", rank);
  #endif
    exit(EXIT_SUCCESS);
  }

  if (size > max_elem) {
    size = max_elem;
  }

  int work_size = max_elem / size;
  int start     = rank * work_size;

  if (rank == (size - 1)) {
    work_size += max_elem - work_size * size;
  }

  int end = start + work_size;

  if (start == 0) {
    sum = FLT_ONE;
  }

  flt_type fact = FLT_ONE;
  for (int i = start; i < end; i++) {
    fact *= i + 1;
    sum += FLT_ONE / fact;
  }

  int          dst      = rank + 1;
  int          src      = rank - 1;

  #ifdef FLT_TYPE_FLOAT
  MPI_Datatype datatype = MPI_FLOAT;
  #elif FLT_TYPE_DOUBLE
  MPI_Datatype datatype = MPI_DOUBLE;
  #elif FLT_TYPE_LONG_DOUBLE
  MPI_Datatype datatype = MPI_LONG_DOUBLE;
  #endif

  if (rank == (size - 1)) {
    if (rank == 0) {
      return sum;
    }

    flt_type prev_fact;
    TRY_MPI(MPI_Recv(&prev_fact, 1, datatype, src, PREV_FACT_TAG, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE));
    prev_fact *= fact;

    flt_type prev_sum;
    TRY_MPI(MPI_Recv(&prev_sum, 1, datatype, src, PREV_SUM_TAG, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE));
    sum /= prev_fact;
    sum += prev_sum;

  } else if (rank == ROOT_RANK) {

    TRY_MPI(MPI_Send(&fact, 1, datatype, dst, PREV_FACT_TAG, MPI_COMM_WORLD));
    TRY_MPI(MPI_Send(&sum, 1, datatype, dst, PREV_SUM_TAG, MPI_COMM_WORLD));

  } else {

    flt_type prev_fact;
    TRY_MPI(MPI_Recv(&prev_fact, 1, datatype, src, PREV_FACT_TAG, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE));
    prev_fact *= fact;
    TRY_MPI(MPI_Send(&prev_fact, 1, datatype, dst, PREV_FACT_TAG, MPI_COMM_WORLD));

    flt_type prev_sum;
    TRY_MPI(MPI_Recv(&prev_sum, 1, datatype, src, PREV_SUM_TAG, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE));
    sum /= prev_fact;
    sum += prev_sum;
    TRY_MPI(MPI_Send(&prev_sum, 1, datatype, dst, PREV_SUM_TAG, MPI_COMM_WORLD));
  }

#endif

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

  int n_digits = get_n_digits(argv[1]);
  if (n_digits < 0) {
    fprintf(stderr, "%s: number is not in an appropriate format\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int extra_digits = log10i(n_digits) + 1;

#ifdef PARALLEL
  TRY_MPI(MPI_Init(NULL, NULL));
  int size = -1;
  int rank = -1;
  TRY_MPI(myMPI_get_rank_and_size(&rank, &size));

  extra_digits += (flt_log10(n_digits) + 1) * n_digits * size + 1;
#endif

  int digits_after_dot = n_digits - 1;
  int n_calc_digits    = n_digits + extra_digits;

  char format[FORMAT_LEN];
  sprintf(format, "e = %%.%d" FLT_PRINT_FORMAT "\n", digits_after_dot);

  flt_type e = calc_exp(n_calc_digits);

#ifdef PARALLEL
  TRY_MPI(MPI_Finalize());
  #ifndef NDEBUG
  fprintf(stderr, "rank %d finalized\n", rank);
  #endif
  if (rank != ROOT_RANK) {
    exit(EXIT_SUCCESS);
  }
  #ifndef NDEBUG
  usleep(450);
  fprintf(stderr, "---\n");
  #endif
#endif

  printf(format, e);
}

