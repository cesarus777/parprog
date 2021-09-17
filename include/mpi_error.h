#pragma once

#include "mpi/mpi.h"

#include <stdio.h>
#include <stdlib.h>



#define myMPI_NOT_INITIALIZED -1
#define myMPI_FINALIZED       -2

#ifdef EXIT_ON_FAIL
  #define myMPI_ERROR_EXIT_ON_FAIL 1
#else
  #define myMPI_ERROR_EXIT_ON_FAIL 0
#endif



const char *myMPI_decode_error(int MPI_ret_val);

#define TRY_MPI(mpi_ret_val)                                                   \
  if ((mpi_ret_val) != MPI_SUCCESS) {                                          \
    fprintf(stderr, "%s:%d '" #mpi_ret_val "' fail: %s with error code %d\n",  \
            __FILE__, __LINE__, myMPI_decode_error(mpi_ret_val),               \
            (mpi_ret_val));                                                    \
    if (myMPI_ERROR_EXIT_ON_FAIL) {                                            \
      MPI_Finalize();                                                          \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  }

int myMPI_get_rank_and_size(int *rank, int *size);

