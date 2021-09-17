set(HEAT_EQUATION_SOURCES
    heat_equation.c
)

add_executable(heat_equation ${HEAT_EQUATION_SOURCES})



if(PARALLEL)
    find_package(MPI REQUIRED)
    target_include_directories(heat_equation PUBLIC ${MPI_INCLUDE_PATH})

    target_compile_definitions(heat_equation PUBLIC PARALLEL)
    target_link_libraries(heat_equation ${MPI_LIBRARIES} ${MPI_SUPPORT_LIBRARIES})
endif()



target_compile_definitions(heat_equation PUBLIC "FLT_TYPE_${FLT_TYPE}")
target_compile_options(heat_equation PUBLIC "-Wall" "-Wextra" "-Wpedantic" "-Werror")
target_link_libraries(heat_equation m)



install(TARGETS heat_equation
    RUNTIME
    DESTINATION heat_equation
    COMPONENT heat_equation
    )

