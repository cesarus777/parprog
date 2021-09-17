set(INTEGRAL_SOURCES
    integral.c
)

add_executable(integral ${INTEGRAL_SOURCES})



target_compile_definitions(integral PUBLIC "FLT_TYPE_${FLT_TYPE}")
target_compile_options(integral PUBLIC "-Wall" "-Wextra" "-Wpedantic" "-Werror")
target_link_libraries(integral m pthread)



install(TARGETS integral
    RUNTIME
    DESTINATION integral
    COMPONENT integral
    )

