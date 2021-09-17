find_package(OpenMP REQUIRED)

set(OpenMP-targets hello_world sum)

foreach(target IN LISTS OpenMP-targets)
  add_executable(${target} "${target}.c")
  target_compile_options(${target} PUBLIC "-Wall" "-Wextra" "-Wpedantic" "-Werror" ${OpenMP_C_FLAGS})
  target_include_directories(${target} PUBLIC ${OpenMP_C_INCLUDE_PATH})
  target_link_libraries(${target} PUBLIC OpenMP::OpenMP_C)
endforeach()

target_link_libraries(sum PUBLIC m)

install(TARGETS ${OpenMP-targets}
    RUNTIME
    DESTINATION OpenMP
    COMPONENT OpenMP
    )

