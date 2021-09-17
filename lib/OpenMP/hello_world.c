#include <limits.h>
#include <omp.h>
#include <stdio.h>
#include <unistd.h>



int main() {
  printf("Hello world!\n");

  int n_printed = 0;

#pragma omp parallel shared(n_printed)
  {
    int tid       = omp_get_thread_num();
    int n_threads = omp_get_num_threads();
    while (n_threads > (tid + 1 + n_printed)) {
      usleep(1);
    }

//#pragma omp critical
    {
      printf("%d\n", tid);
      fflush(stdout);
      n_printed++;
    }
  }
}
