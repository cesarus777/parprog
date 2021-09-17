if(ENABLED_PROJECTS STREQUAL "ALL")
  set(ENABLED_PROJECTS
      heat_equation
      exp_calc
      integral
      OpenMP
      ray_tracer)
endif()

if(NOT "support" IN_LIST "${ENABLED_PROJECTS}")
  set(ENABLED_PROJECTS "support" ${ENABLED_PROJECTS})
endif()

message("-- ENABLED_PROJECTS: ${ENABLED_PROJECTS}")



if(NOT DEFINED FLT_TYPE)
  message("-- FLT_TYPE wasn't declarated. Using 'double'")
  set(FLT_TYPE "DOUBLE")
endif()

if(DEFINED FLT_TYPE)
  message("-- FLT_TYPE set to ${FLT_TYPE}")
endif()

if(PARALLEL)
  set(MPI_SUPPORT_LIBRARIES support_mpi)
endif()



foreach(SUBPROJECT IN LISTS ENABLED_PROJECTS)
  add_subdirectory(${SUBPROJECT})
endforeach()

