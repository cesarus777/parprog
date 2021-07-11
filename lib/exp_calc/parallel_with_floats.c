#include "flt_type.h"

#ifdef PARALLEL
  #include "mpi_error.h"
  #include "mpi/mpi.h"
#endif

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



#ifdef PARALLEL

enum {
  ELEM_TAG,
  INV_PREV_FACT_TAG,
  PREV_SUM_TAG,
  SUM_TAG,
};

#endif



enum { FORMAT_LEN = 24 };



// using Stirling's approximation find the least n: [1 / (n + 1)!] < [0.5 * 10^(n_digits)]
int count_max_elem(int n_digits) {
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
  flt_type sum = FLT_ZERO;
  int max_elem = count_max_elem(n_digits);

#ifndef PARALLEL

  uint64_t fact_i = 1;
  for(int i = 0; i < max_elem; i++) {
    fact_i *= i + 1;
    sum += FLT_ONE / (flt_type) fact_i;
  }

#else

  MPI_TRY(MPI_Init(NULL, NULL));
  int size = -1, rank = -1;
  MPI_TRY(MPI_Comm_size(MPI_COMM_WORLD, &size));
  assert(size > 0);
  MPI_TRY(MPI_Comm_rank(MPI_COMM_WORLD, &rank));
  assert(rank >= 0);

  if(rank >= max_elem) {
    MPI_TRY(MPI_Finalize());
    exit(EXIT_SUCCESS);
  }

  if(size > max_elem)
    size = max_elem;

  int work_size = max_elem / size;
  int start = rank * work_size;

  if(rank == (size - 1))
    work_size += max_elem - work_size * size;

  int end = start + work_size;

  if(start == 0) {
    start = 1;
  }

  flt_type elem = FLT_ONE;
  for(int i = start; i < end; i++) {
    elem /= i;
    sum += elem;
  }

  int dst = rank + 1;
  int src = rank - 1;

#ifdef FLT_TYPE_FLOAT
  MPI_Datatype datatype = MPI_FLOAT;
#elif FLT_TYPE_DOUBLE
  MPI_Datatype datatype = MPI_DOUBLE;
#elif FLT_TYPE_LONG_DOUBLE
  MPI_Datatype datatype = MPI_LONG_DOUBLE;
#endif

  if(rank == (size - 1)) {
    if(rank == 0) {
      MPI_TRY(MPI_Finalize());
      return sum + FLT_ONE;
    }

    flt_type inv_prev_fact;
    MPI_TRY(MPI_Recv(&inv_prev_fact, 1, datatype, src, INV_PREV_FACT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
    inv_prev_fact *= elem;

    flt_type prev_sum;
    MPI_TRY(MPI_Recv(&prev_sum, 1, datatype, src, PREV_SUM_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
    sum *= inv_prev_fact;
    sum += prev_sum;

    MPI_TRY(MPI_Finalize());

  } else if(rank == 0) {

    MPI_TRY(MPI_Send(&elem, 1, datatype, dst, ELEM_TAG, MPI_COMM_WORLD));
    MPI_TRY(MPI_Send(&sum, 1, datatype, dst, SUM_TAG, MPI_COMM_WORLD));

    MPI_TRY(MPI_Finalize());
    exit(EXIT_SUCCESS);

  } else {

    flt_type inv_prev_fact;
    MPI_TRY(MPI_Recv(&inv_prev_fact, 1, datatype, src, INV_PREV_FACT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
    inv_prev_fact *= elem;
    MPI_TRY(MPI_Send(&inv_prev_fact, 1, datatype, dst, INV_PREV_FACT_TAG, MPI_COMM_WORLD));

    flt_type prev_sum;
    MPI_TRY(MPI_Recv(&prev_sum, 1, datatype, src, PREV_SUM_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
    sum *= inv_prev_fact;
    sum += prev_sum;
    MPI_TRY(MPI_Send(&prev_sum, 1, datatype, dst, PREV_SUM_TAG, MPI_COMM_WORLD));

    MPI_TRY(MPI_Finalize());
    exit(EXIT_SUCCESS);
  }
  /*
  if(rank == 0) {
    flt_type buf = FLT_ZERO;
    for(int i = 1; i < size; i++) {
#ifdef FLT_TYPE_FLOAT
      MPI_Datatype datatype = MPI_FLOAT;
#elif FLT_TYPE_DOUBLE
      MPI_Datatype datatype = MPI_DOUBLE;
#elif FLT_TYPE_LONG_DOUBLE
      MPI_Datatype datatype = MPI_LONG_DOUBLE;
#endif
      MPI_TRY(MPI_Recv(&buf, 1, datatype, i, SUM_SYNC_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
      sum += buf;
    }
  } else {
#ifdef FLT_TYPE_FLOAT
    MPI_Datatype datatype = MPI_FLOAT;
#elif FLT_TYPE_DOUBLE
    MPI_Datatype datatype = MPI_DOUBLE;
#elif FLT_TYPE_LONG_DOUBLE
    MPI_Datatype datatype = MPI_LONG_DOUBLE;
#endif
    MPI_TRY(MPI_Send(&sum, 1, datatype, 0, SUM_SYNC_TAG, MPI_COMM_WORLD));
    MPI_TRY(MPI_Finalize());
    exit(EXIT_SUCCESS);
  }
  */

#endif

  return sum + FLT_ONE;
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

  int n_digits = get_n_digits(argv[1]);
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

