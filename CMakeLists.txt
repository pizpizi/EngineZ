cmake_minimum_required(VERSION 3.10)
project(Engine)

set(CMAKE_CXX_STANDARD 20)

# Add GLFW
add_subdirectory(dependencies/GLFW)
find_package(Vulkan REQUIRED)

# Add your executable

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HDR_FILES "${CMAKE_SOURCE_DIR}/include/*.h")


add_library(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan glfw)

add_executable(test
    test/main.cpp
)
target_link_libraries(test PRIVATE ${PROJECT_NAME})
