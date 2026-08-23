cmake_minimum_required(VERSION 3.20)
project(EngineZ)

option(BUILD_EXAMPLES "Build engine examples" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/logz)
add_subdirectory(dependencies/imgui)

add_library(enginez 
    src/ez_engine.cpp
    src/ez_vulkan_backend.cpp
    src/ez_window.cpp
)

target_include_directories(enginez
    PUBLIC include dependencies/vulkan/include
    PRIVATE src
)

target_link_libraries(imgui
    glfw
)

target_link_libraries(enginez
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/vulkan/lib/libvulkan.so.1
        glfw
        logz
        imgui
)

target_compile_features(enginez PUBLIC cxx_std_23)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()