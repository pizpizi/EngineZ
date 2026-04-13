cmake_minimum_required(VERSION 3.10)

project("EngineZ")
option(ENGINEZ_BUILD_EXAMPLES "Build engine examples" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("${CMAKE_SOURCE_DIR}/dependencies/glfw")
add_subdirectory("${CMAKE_SOURCE_DIR}/dependencies/logz")
find_package(Vulkan REQUIRED)

add_library(enginez src/engine.cpp src/vulkan/vulkan_utilities.cpp src/vulkan/vulkan_backend.cpp src/vulkan/vulkan_window)

target_include_directories(enginez
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(enginez
    PUBLIC 
        Vulkan::Vulkan 
        glfw
        logz
)

if(ENGINEZ_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()