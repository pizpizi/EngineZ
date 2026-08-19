cmake_minimum_required(VERSION 3.1)
project(EngineZ)

option(BUILD_EXAMPLES "Build engine examples" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/logz)
find_package(Vulkan REQUIRED)

add_library(enginez 
    src/engine.cpp
    src/vulkan/vulkan_utilities.cpp
    src/vulkan/vulkan_backend.cpp
    src/vulkan/vulkan_window
)

target_include_directories(enginez
    PUBLIC include
    PRIVATE src
)

target_link_libraries(enginez
    PUBLIC 
        Vulkan::Vulkan 
        glfw
        logz
)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()