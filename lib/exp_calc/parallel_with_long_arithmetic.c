#include "flt_type.h"

#ifdef PARALLEL
  #include "mpi/mpi.h"
  #include "mpi_error.h"
#endif

#include <gmp.h>

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



//#undef NDEBUG

#ifdef PARALLEL

enum {
  MP_PREC_TAG,
  MP_SIZE_TAG,
  MP_EXP_TAG,
  MP_D_TAG,
  SUM_SYNC_TAG,
};

enum { ROOT_RANK = 0 };

#endif



enum { BASE = 10 };
enum { DEFAULT_N_DIGITS = 100 };



int log10i(int x) { return (int) (flt_log10(x) + 1 / (flt_type) 2); }



#ifdef PARALLEL

int mpf_recv_mpi(mpf_t x, int src) {
  int initialized = 0;
  int finalized   = 1;
  TRY_MPI(MPI_Initialized(&initialized));
  TRY_MPI(MPI_Finalized(&finalized));

  if (!initialized || finalized) {
    return -1;
  }

  MPI_Datatype exp_datatype = MPI_LONG;
  if (__GMP_MP_SIZE_T_INT) {
    exp_datatype = MPI_INT;
  }

  #ifdef __GMP_SHORT_LIMB
  MPI_Datatype limb_datatype = MPI_UNSIGNED;
  #else
    #ifdef _LONG_LONG_LIMB
  MPI_Datatype limb_datatype = MPI_UNSIGNED_LONG_LONG;
    #else
  MPI_Datatype limb_datatype = MPI_UNSIGNED_LONG;
    #endif
  #endif

  TRY_MPI(MPI_Recv(&x->_mp_prec, 1, MPI_INT, src, MP_PREC_TAG, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE));
  TRY_MPI(MPI_Recv(&x->_mp_size, 1, MPI_INT, src, MP_SIZE_TAG, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE));
  TRY_MPI(MPI_Recv(&x->_mp_exp, 1, exp_datatype, src, MP_EXP_TAG, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE));

  x->_mp_d = malloc((x->_mp_prec + 1) * sizeof(mp_limb_t));
  assert(x->_mp_d);
  TRY_MPI(MPI_Recv(x->_mp_d, x->_mp_prec + 1, limb_datatype, src, MP_D_TAG,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE));

  return 0;
}

  #define try_recv_mpf(x, src)                                                 \
    if (mpf_recv_mpi((x), (src)) != 0) {                                       \
      TRY_MPI(MPI_Finalize());                                                     \
      exit(EXIT_FAILURE);                                                      \
    }



int mpf_send_mpi(mpf_t x, int dst) {
  int initialized = 0;
  int finalized   = 1;
  TRY_MPI(MPI_Initialized(&initialized));
  TRY_MPI(MPI_Finalized(&finalized));

  if (!initialized || finalized) {
    return -1;
  }

  MPI_Datatype exp_datatype = MPI_LONG;
  if (__GMP_MP_SIZE_T_INT) {
    exp_datatype = MPI_INT;
  }

  #ifdef __GMP_SHORT_LIMB
  MPI_Datatype limb_datatype = MPI_UNSIGNED;
  #else
    #ifdef _LONG_LONG_LIMB
  MPI_Datatype limb_datatype = MPI_UNSIGNED_LONG_LONG;
    #else
  MPI_Datatype limb_datatype = MPI_UNSIGNED_LONG;
    #endif
  #endif

  TRY_MPI(MPI_Send(&x->_mp_prec, 1, MPI_INT, dst, MP_PREC_TAG, MPI_COMM_WORLD));
  TRY_MPI(MPI_Send(&x->_mp_size, 1, MPI_INT, dst, MP_SIZE_TAG, MPI_COMM_WORLD));
  TRY_MPI(MPI_Send(&x->_mp_exp, 1, exp_datatype, dst, MP_EXP_TAG, MPI_COMM_WORLD));
  TRY_MPI(MPI_Send(x->_mp_d, x->_mp_prec + 1, limb_datatype, dst, MP_D_TAG,
               MPI_COMM_WORLD));

  return 0;
}

  #define try_send_mpf(x, src)                                                 \
    if (mpf_send_mpi((x), (src)) != 0) {                                       \
      TRY_MPI(MPI_Finalize());                                                     \
      exit(EXIT_FAILURE);                                                      \
    }

#endif



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



void calc_exp(mpf_t sum, int n_digits) {
  int max_elem = count_max_elem(n_digits);
  mpf_init_set_ui(sum, 0);

#ifdef PARALLEL
  int size = -1;
  int rank = -1;

  TRY_MPI(myMPI_get_rank_and_size(&rank, &size));

  if (rank >= max_elem) {
    TRY_MPI(MPI_Finalize());
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
#else
  int start = 0;
  int end = max_elem;
#endif

  if (start == 0) {
    start = 1;
    mpf_set_ui(sum, 1);
  }

  mpf_t elem;
  mpf_init_set_ui(elem, 1);
  for (int i = start; i < end; i++) {
    mpf_div_ui(elem, elem, i);
    mpf_add(sum, sum, elem);
  }

#ifdef PARALLEL
  int dst = rank + 1;
  int src = rank - 1;

  if (rank == (size - 1)) {
    if (rank == 0) {
      return;
    }

    mpf_t inv_prev_fact;
    try_recv_mpf(inv_prev_fact, src);
    mpf_mul(inv_prev_fact, inv_prev_fact, elem);

    mpf_t prev_sum;
    try_recv_mpf(prev_sum, src);
    mpf_mul(sum, sum, inv_prev_fact);
    mpf_add(sum, sum, prev_sum);

    mpf_clears(elem, inv_prev_fact, prev_sum, NULL);

  } else if (rank == 0) {

    try_send_mpf(elem, dst);
    try_send_mpf(sum, dst);

    TRY_MPI(MPI_Finalize());
  #ifndef NDEBUG
    fprintf(stderr, "rank %d finalized\n", rank);
  #endif
    mpf_clears(sum, elem, NULL);
    exit(EXIT_SUCCESS);

  } else {

    mpf_t inv_prev_fact;
    try_recv_mpf(inv_prev_fact, src);
    mpf_mul(inv_prev_fact, inv_prev_fact, elem);
    try_send_mpf(inv_prev_fact, dst);

    mpf_t prev_sum;
    try_recv_mpf(prev_sum, src);
    mpf_mul(sum, sum, inv_prev_fact);
    mpf_add(sum, sum, prev_sum);
    try_send_mpf(prev_sum, dst);

    TRY_MPI(MPI_Finalize());
  #ifndef NDEBUG
    fprintf(stderr, "rank %d finalized\n", rank);
  #endif
    mpf_clears(sum, elem, inv_prev_fact, prev_sum, NULL);
    exit(EXIT_SUCCESS);
  }
#endif
}



int get_n_digits(const char *str) {
  if (str == NULL) {
    return DEFAULT_N_DIGITS;
  }
  char *endptr;
  errno  = 0;
  long n = strtol(str, &endptr, BASE);
  if (((errno == ERANGE) && (n == LONG_MAX || n == LONG_MIN)) || (n < 0) ||
      (*endptr != '\0')) {
    return -1;
  }

  return n;
}



int main(int argc, char *argv[]) {
  if (argc > 2) {
    fprintf(stderr, "%s: too much arguments\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  int n_digits = -1;
  if (argc == 2) {
    n_digits = get_n_digits(argv[1]);
  } else {
    n_digits = DEFAULT_N_DIGITS;
  }

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

  int            n_calc_digits = n_digits + extra_digits;
  const flt_type epsilon       = 0.5;
  int            prec          = n_calc_digits * flt_log2(BASE) + epsilon;
  mpf_set_default_prec(prec);
  mpf_t sum;
  mpf_init(sum);
  calc_exp(sum, n_calc_digits);

#ifdef PARALLEL
  TRY_MPI(MPI_Finalize());
  #ifndef NDEBUG
  fprintf(stderr, "rank %d finalized\n", size - 1);
  usleep(450);
  fprintf(stderr, "---\n");
  #endif
#endif

  gmp_printf("e = %.*Ff\n", n_digits - 1, sum);
  mpf_clear(sum);
}

