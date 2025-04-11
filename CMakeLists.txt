cmake_minimum_required(VERSION 3.20)

project(imgui_sdl2_vulkan LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libs/imgui)
add_subdirectory(libs/imnodes)

# ================
add_executable(imgui_sdl2_vulkan_example "example/example_sdl2_vulkan/main.cpp")
target_link_libraries(imgui_sdl2_vulkan_example PUBLIC imgui_sdl2_vulkan)  

add_subdirectory("example/example_imnodes")