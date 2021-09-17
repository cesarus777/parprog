if(PARALLEL)
  find_package(MPI REQUIRED)

  set(mpi_support-targets mpi_error)
  set(MPI_SUPPORT_LIBRARIES support_mpi)
  add_library(${MPI_SUPPORT_LIBRARIES} OBJECT ${mpi_support-targets})
  target_compile_options(${MPI_SUPPORT_LIBRARIES} PUBLIC "-Wall" "-Wextra" "-Wpedantic" "-Werror")
  target_compile_definitions(${MPI_SUPPORT_LIBRARIES} PUBLIC "FLT_TYPE_${FLT_TYPE}")
endif()
