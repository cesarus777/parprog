if(ENABLED_PROJECTS STREQUAL "ALL")
  set(ENABLED_PROJECTS
      "exp_calc")
endif()



if(NOT DEFINED FLT_TYPE)
  message("-- FLT_TYPE wasn't declarated. Using 'double'")
  set(FLT_TYPE "DOUBLE")
endif()

if(DEFINED FLT_TYPE)
  message("-- FLT_TYPE set to ${FLT_TYPE}")
endif()



foreach(SUBPROJECT IN LISTS ENABLED_PROJECTS)
  add_subdirectory(${SUBPROJECT})
endforeach()

