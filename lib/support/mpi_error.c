#include "mpi_error.h"

#include "mpi/mpi.h"

#include <assert.h>



const char *myMPI_decode_error(int MPI_ret_val) {
  switch (MPI_ret_val) {
  case myMPI_NOT_INITIALIZED:
    return "MPI isn't initialized";
  case myMPI_FINALIZED:
    return "MPI is finalized";
  case MPI_ERR_BUFFER:
    return "Invalid buffer pointer";
  case MPI_ERR_COUNT:
    return "Invalid count argument";
  case MPI_ERR_TYPE:
    return "Invalid datatype argument";
  case MPI_ERR_COMM:
    return "Invalid communicator";
  case MPI_ERR_RANK:
    return "Invalid rank";
  default:
    return "Unknown error";
  }
}

int myMPI_get_rank_and_size(int *rank, int *size) {
  assert(rank);
  assert(size);

  int initialized = 0;
  int finalized   = 1;
  TRY_MPI(MPI_Initialized(&initialized));
  TRY_MPI(MPI_Finalized(&finalized));

  if (!initialized) {
    return myMPI_NOT_INITIALIZED;
  }

  if (finalized) {
    return myMPI_FINALIZED;
  }

  TRY_MPI(MPI_Comm_size(MPI_COMM_WORLD, size));
  assert(*size > 0);
  TRY_MPI(MPI_Comm_rank(MPI_COMM_WORLD, rank));
  assert(*rank >= 0);

  return MPI_SUCCESS;
}

