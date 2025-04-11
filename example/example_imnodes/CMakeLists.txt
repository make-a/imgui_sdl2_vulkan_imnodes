cmake_minimum_required(VERSION 3.20)

project(examples LANGUAGES CXX)

add_executable(example_imnodes)

file(GLOB_RECURSE SRC "${CMAKE_CURRENT_LIST_DIR}/app/*.cpp")
 
target_sources(
    example_imnodes
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp

    ${SRC}
)

target_link_libraries(example_imnodes PUBLIC imgui_sdl2_vulkan imnodes)
target_include_directories(example_imnodes PUBLIC "${CMAKE_SOURCE_DIR}/libs/imnodes") 