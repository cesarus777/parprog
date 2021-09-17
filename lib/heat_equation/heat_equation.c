#include "initial_conditions.h"

#include "flt_type.h"

#ifdef PARALLEL
  #include "mpi/mpi.h"
  #include "mpi_error.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>



#ifdef PARALLEL
enum {
  ROOT_SYNC_TAG,
  ROOT_SYNC_PREV_TAG,
  ROOT_SYNC_NEXT_TAG,
};
#endif



flt_type *calc_convection_diffusion(const conditions_t *conditions);



int main() {
  conditions_t conditions = {x_begin, x_end, x_step, t_begin, t_end,
                             t_step,  a,     &phi,   &psi,    &f};

  clock_t global_time_begin = clock();

  flt_type *num_solution = calc_convection_diffusion(&conditions);

  for (int t = 0; t < T_STEPS; t++) {
    for (int x = 0; x < X_STEPS; x++) {
      printf("%lf %lf %lf\n", conditions.x_begin + x * conditions.x_step,
             conditions.t_begin + t * conditions.t_step,
             num_solution[X_STEPS * t + x]);
    }
  }

  free(num_solution);

  clock_t global_time_end = clock();

  double global_elapsed_time =
      (global_time_end - global_time_begin) / (double) CLOCKS_PER_SEC;
  fprintf(stderr, "global_elapsed_time: %lfs\n", global_elapsed_time);

  fprintf(stderr, "process with rank 0 exited successfully\n");
}


//flt_type *calc_convection_diffusion(const conditions_t *conditions) {
//  flt_type *solution = malloc(X_STEPS * T_STEPS * sizeof(double));
//
//  for (int t = 0; t < T_STEPS; t++) {
//    for (int x = 0; x < X_STEPS; x++) {
//      // border
//      if (t == 0) {
//        solution[X_STEPS * t + x] =
//            conditions->phi(conditions->x_begin + x * conditions->x_step);
//        continue;
//      }
//
//      // border
//      if (x == 0) {
//        solution[X_STEPS * t + x] =
//            conditions->psi(conditions->t_begin + t * conditions->t_step);
//        continue;
//      }
//
//      const int      k_next_m = X_STEPS * t + x;
//      const int      k_prev_m = X_STEPS * (t - 2) + x;
//      const int      k_m      = X_STEPS * (t - 1) + x;
//      const int      k_m_next = X_STEPS * (t - 1) + x + 1;
//      const int      k_m_prev = X_STEPS * (t - 1) + x - 1;
//      const flt_type f_k_m =
//          conditions->f(conditions->x_begin + x * conditions->x_step,
//                        conditions->t_begin + t * conditions->t_step);
//
//      // fallback to angle scheme
//      if (t == 1 || x == X_STEPS - 1) {
//        solution[k_next_m] = (f_k_m - (solution[k_m] - solution[k_m_prev]) /
//                                          conditions->x_step) *
//                                 conditions->t_step +
//                             solution[k_m];
//        continue;
//      }
//
//      // cross itself
//      solution[k_next_m] = (f_k_m - (solution[k_m_next] - solution[k_m_prev]) /
//                                        (2.0 * conditions->x_step)) *
//                               2.0 * conditions->t_step +
//                           solution[k_prev_m];
//    }
//  }
//
//  return solution;
//}



flt_type *calc_convection_diffusion(const conditions_t *conditions) {
#ifndef PARALLEL
  flt_type *solution = malloc(X_STEPS * T_STEPS * sizeof(double));

  for (int t = 0; t < T_STEPS; t++) {
    for (int x = 0; x < X_STEPS; x++) {
      // border
      if (t == 0) {
        solution[X_STEPS * t + x] =
            conditions->phi(conditions->x_begin + x * conditions->x_step);
        continue;
      }

      // border
      if (x == 0) {
        solution[X_STEPS * t + x] =
            conditions->psi(conditions->t_begin + t * conditions->t_step);
        continue;
      }

      const int      k_next_m = X_STEPS * t + x;
      const int      k_prev_m = X_STEPS * (t - 2) + x;
      const int      k_m      = X_STEPS * (t - 1) + x;
      const int      k_m_next = X_STEPS * (t - 1) + x + 1;
      const int      k_m_prev = X_STEPS * (t - 1) + x - 1;
      const flt_type f_k_m =
          conditions->f(conditions->x_begin + x * conditions->x_step,
                        conditions->t_begin + t * conditions->t_step);

      // fallback to angle scheme
      if (t == 1 || x == X_STEPS - 1) {
        solution[k_next_m] = (f_k_m - (solution[k_m] - solution[k_m_prev]) /
                                          conditions->x_step) *
                                 conditions->t_step +
                             solution[k_m];
        continue;
      }

      // cross itself
      solution[k_next_m] = (f_k_m - (solution[k_m_next] - solution[k_m_prev]) /
                                        (2.0 * conditions->x_step)) *
                               2.0 * conditions->t_step +
                           solution[k_prev_m];
    }
  }

  return solution;
#else
  TRY_MPI(MPI_Init(NULL, NULL));
  int rank = -1;
  int size = -1;
  TRY_MPI(myMPI_get_rank_and_size(&rank, &size));

  if (rank >= X_STEPS) {
    TRY_MPI(MPI_Finalize());
    exit(EXIT_SUCCESS);
  }

  if (size > X_STEPS) {
    size = X_STEPS;
  }

  int work_size = X_STEPS / size;
  int start     = rank * work_size;

  if (rank == (size - 1)) {
    work_size += X_STEPS - work_size * size;
  }

  int end = start + work_size;

  int alloc_size = X_STEPS;
  if (rank == (size - 1)) {
    alloc_size += 1;
  } else if (rank != 0) {
    alloc_size += 2;
  }

  flt_type *local_solution = malloc(alloc_size * T_STEPS * sizeof(flt_type));
  assert(local_solution);

  for (int t = 0; t < T_STEPS; t++) {
    for (int x = 0; x < end - start; x++) {
      if (rank != 0) {
        x++;
      }

      // border
      if (t == 0) {
        local_solution[X_STEPS * t + x] =
            conditions->phi(conditions->x_begin + x * conditions->x_step);
        continue;
      }

      // border
      if (x == 0) {
        local_solution[X_STEPS * t + x] =
            conditions->psi(conditions->t_begin + t * conditions->t_step);
        continue;
      }

      const int      k_next_m = work_size * t + x;
      const int      k_prev_m = work_size * (t - 2) + x;
      const int      k_m      = work_size * (t - 1) + x;
      const int      k_m_next = work_size * (t - 1) + x + 1;
      const int      k_m_prev = work_size * (t - 1) + x - 1;
      const flt_type f_k_m =
          conditions->f(conditions->x_begin + x * conditions->x_step,
                        conditions->t_begin + t * conditions->t_step);

      // fallback to angle scheme
      if ((t == 1) || (x == (X_STEPS - 1))) {
        local_solution[k_next_m] =
            (f_k_m - (local_solution[k_m] - local_solution[k_m_prev]) /
                         conditions->x_step) *
                conditions->t_step +
            local_solution[k_m];
        continue;
      }

      // cross itself
      local_solution[k_next_m] =
          (f_k_m - conditions->a *
                       (local_solution[k_m_next] - local_solution[k_m_prev]) /
                       (2.0 * conditions->x_step)) *
              2.0 * conditions->t_step +
          local_solution[k_prev_m];
    }

    MPI_Datatype datatype =
  #ifdef FLT_TYPE_FLOAT
        MPI_FLOAT;
  #elif FLT_TYPE_DOUBLE
        MPI_DOUBLE;
  #elif FLT_TYPE_LONG_DOUBLE
        MPI_LONG_DOUBLE;
  #endif

    // sync body
    if (rank == 0) {
      int offset = X_STEPS / size;

      for (int i = 1; i < size; i++) {
        int n_elems;
        if (i != (size - 1)) {
          n_elems = X_STEPS / size;
        } else {
          n_elems = X_STEPS - (X_STEPS / size) * (size - 1);
        }

        TRY_MPI(MPI_Recv(&local_solution[X_STEPS * t + offset], n_elems,
                         datatype, i, ROOT_SYNC_TAG, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE));
        fprintf(stderr, "%4d:Recv:body:datasize=%3d:rank=%2d\n", t, n_elems,
                rank);
        fflush(stderr);

        offset += n_elems;
      }
    } else {
      TRY_MPI(MPI_Send(local_solution + 1, work_size, datatype, 0,
                       ROOT_SYNC_TAG, MPI_COMM_WORLD));
      fprintf(stderr, "%4d:Send:body:datasize=%3d:rank=%2d\n", t, work_size,
              rank);
      fflush(stderr);
    }

    // sync prev
    if (rank == 0) {
      int offset = X_STEPS / size;

      for (int i = 1; i < size; i++) {
        int n_elems;
        if (i != (size - 1)) {
          n_elems = X_STEPS / size;
        } else {
          n_elems = X_STEPS - (X_STEPS / size) * (size - 1);
        }

        TRY_MPI(MPI_Send(&local_solution[X_STEPS * t + offset - 1], 1, datatype,
                         i, ROOT_SYNC_PREV_TAG, MPI_COMM_WORLD));
        fprintf(stderr, "%4d:Send:prev:rank=%d\n", t, rank);
        fflush(stderr);

        offset += n_elems;
      }
    } else {
      TRY_MPI(MPI_Recv(local_solution, 1, datatype, 0, ROOT_SYNC_PREV_TAG,
                       MPI_COMM_WORLD, MPI_STATUS_IGNORE));
      fprintf(stderr, "%4d:Recv:prev:rank=%d\n", t, rank);
      fflush(stderr);
    }

    // sync next
    if (rank == 0) {
      int offset = X_STEPS / size;

      for (int i = 1; i < size - 1; i++) {
        int n_elems;
        if (i != (size - 1)) {
          n_elems = X_STEPS / size;
        } else {
          n_elems = X_STEPS - (X_STEPS / size) * (size - 1);
        }

        TRY_MPI(MPI_Recv(&local_solution[X_STEPS * t + offset + n_elems], 1,
                         datatype, i, ROOT_SYNC_NEXT_TAG, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE));
        fprintf(stderr, "%4d:Recv:next:rank=%d\n", t, rank);
        fflush(stderr);

        offset += n_elems;
      }
    } else if (rank != (size - 1)) {
      TRY_MPI(MPI_Send(&local_solution[alloc_size - 1], 1, datatype, 0,
                       ROOT_SYNC_NEXT_TAG, MPI_COMM_WORLD));
      fprintf(stderr, "%4d:Send:next:rank=%d\n", t, rank);
      fflush(stderr);
    }
  }

  fprintf(stderr, "process with rank %d finished successfully\n", rank);
  fflush(stderr);

  TRY_MPI(MPI_Finalize());

  if (rank != 0) {
    free(local_solution);
    fprintf(stderr, "process with rank %d exited successfully\n", rank);
    fflush(stderr);
    exit(EXIT_SUCCESS);
  }

  return local_solution;
#endif
}

