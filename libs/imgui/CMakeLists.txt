

set(CMAKE_PREFIX_PATH "D:/Program Files/VulkanSDK/1.4.309.0")
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)
message(STATUS "Vulkan SDK found at:  ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES}")
add_library(imgui_sdl2_vulkan STATIC)

file(GLOB_RECURSE SRC_IMGUI "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE SRC_IMGUI_BACKEND "${CMAKE_CURRENT_LIST_DIR}/backend/*.cpp")
file(GLOB_RECURSE HEADER_IMGUI "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB_RECURSE HEADER_IMGUI_BACKEND "${CMAKE_CURRENT_LIST_DIR}/backend/*.h")

target_include_directories(
    imgui_sdl2_vulkan
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    # PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/backend
)
target_sources(imgui_sdl2_vulkan PRIVATE ${SRC_IMGUI} ${SRC_IMGUI_BACKEND})
target_link_libraries(imgui_sdl2_vulkan PUBLIC ${Vulkan_LIBRARIES} ${SDL2_LIBRARIES})