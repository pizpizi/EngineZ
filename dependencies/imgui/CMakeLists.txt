cmake_minimum_required(VERSION 3.10)

project("imgui")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imgui
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_tables.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
)

target_include_directories(imgui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/backends
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/
)