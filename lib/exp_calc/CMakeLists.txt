set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/")

find_package(GMP REQUIRED)

set(exp_calc-targets
  sequential
  parallel_with_floats
  parallel_with_long_arithmetic
  )

set(exp_calc-parallel_targets
  parallel_with_floats
  parallel_with_long_arithmetic
  )



foreach(target IN LISTS exp_calc-targets)
  add_executable(${target} "${target}.c")
  target_compile_options(${target} PUBLIC "-Wall" "-Wextra" "-Wpedantic" "-Werror")
  target_link_libraries(${target} m)
endforeach()



foreach(target IN LISTS exp_calc-targets)
  target_compile_definitions(${target} PUBLIC "FLT_TYPE_${FLT_TYPE}")
endforeach()



if(PARALLEL)
  find_package(MPI REQUIRED)
  foreach(target IN LISTS exp_calc-parallel_targets)
    target_include_directories(${target} PUBLIC ${MPI_INCLUDE_PATH})
    target_compile_definitions(${target} PUBLIC PARALLEL)
    target_link_libraries(${target} ${MPI_LIBRARIES})
  endforeach()
endif()



target_include_directories(parallel_with_long_arithmetic PUBLIC ${GMP_INCLUDE_PATH})
target_link_libraries(parallel_with_long_arithmetic ${GMP_LIBRARIES})



install(TARGETS sequential 
                parallel_with_floats 
                parallel_with_long_arithmetic
    RUNTIME
    DESTINATION exp_calc
    COMPONENT exp_calc
    )

