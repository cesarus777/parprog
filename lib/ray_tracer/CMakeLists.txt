set(RT_SOURCES
    colors.c
    geometry.c
    scene.c
    ray_casting.c
    ray_tracer.c
)

add_executable(ray_tracer ${RT_SOURCES})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/")

find_package(BSD REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
target_include_directories(ray_tracer PUBLIC ${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})



if(PARALLEL)
    find_package(MPI REQUIRED)
    target_include_directories(ray_tracer PUBLIC ${MPI_INCLUDE_PATH})

    target_compile_definitions(ray_tracer PUBLIC DRAW_PARALLEL)
    target_link_libraries(ray_tracer ${MPI_LIBRARIES} ${MPI_SUPPORT_LIBRARIES})
endif()



target_compile_definitions(ray_tracer PUBLIC "FLT_TYPE_${FLT_TYPE}")
target_compile_options(ray_tracer PUBLIC "-Wall" "-Wextra" "-Wpedantic" "-Werror")
target_link_libraries(ray_tracer m ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${BSD_LIBRARIES})



install(TARGETS ray_tracer
    RUNTIME
    DESTINATION ray_tracer
    COMPONENT ray_tracer
    )

install(FILES scenes.rtr
    DESTINATION ray_tracer
    COMPONENT ray_tracer
    )
