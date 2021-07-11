#pragma once

#include "mpi/mpi.h"



#ifdef EXIT_ON_FAIL
#define MPI_ERROR_EXIT_ON_FAIL 1
#else
#define MPI_ERROR_EXIT_ON_FAIL 0
#endif



const char *MPI_decode_error(int MPI_ret_val) {
  switch(MPI_ret_val) {
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

  return "Unknown error";
}



#define MPI_TRY(MPI_ret_val) \
    if((MPI_ret_val) != MPI_SUCCESS) { \
      fprintf(stderr, \
              "%s:%d '" #MPI_ret_val "' fail: %s with error code %d\n", \
              __FILE__, \
              __LINE__, \
              MPI_decode_error(MPI_ret_val), \
              (MPI_ret_val)); \
      if(MPI_ERROR_EXIT_ON_FAIL) { \
        MPI_Finalize(); \
        exit(EXIT_FAILURE); \
      } \
    }
