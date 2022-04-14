cmake_minimum_required(VERSION 3.10.0)

project(MCenter)

AUX_SOURCE_DIRECTORY(./imgui IMGUI_SRCS)

set(SRCS main.cxx application.cxx mobilemanager.cxx utils.cxx transform.cxx)

if (WIN32)
    set(LIBS SDL2d SDL2maind)
else()
    set(LIBS SDL2 imobiledevice-1.0 plist-2.0)
endif()

add_executable(${PROJECT_NAME} ${SRCS} ${IMGUI_SRCS})
target_link_libraries(${PROJECT_NAME} ${LIBS})

