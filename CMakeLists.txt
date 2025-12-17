cmake_minimum_required(VERSION 3.10)

# if(WIN32)
#     set(CMAKE_MAKE_PROGRAM "${CMAKE_SOURCE_DIR}/tools/ninja.exe")
# endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()


if(WIN32)
    set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
    set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")
    set(CMAKE_RC_COMPILER "C:/Program Files/LLVM/bin/llvm-rc.exe")
elseif(UNIX)
    # Adjust to your Linux toolchain path
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_RC_COMPILER "/usr/bin/windres") # or skip if not needed
endif()

add_compile_options(-Werror=return-type)

project(Engine)

set(CMAKE_CXX_STANDARD 20)

# Add GLFW
add_subdirectory("${CMAKE_SOURCE_DIR}/dependencies/glfw")
find_package(Vulkan REQUIRED)

# Add your executable

file(GLOB_RECURSE HDR_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

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
