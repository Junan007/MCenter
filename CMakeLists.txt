cmake_minimum_required(VERSION 3.10.0)

project(project1)

set( ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)

if (WIN32)
    include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/SDL2
    )
    link_directories(${PROJECT_SOURCE_DIR}/lib)
else()
    include_directories(
        /usr/local/include
        /usr/local/include/SDL2
    )
    link_directories(/usr/local/lib)
endif()

add_subdirectory(src)
