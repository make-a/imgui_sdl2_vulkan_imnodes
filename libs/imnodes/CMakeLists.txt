
add_library(imnodes STATIC)
target_sources(
    imnodes
    PRIVATE
    imnodes.cpp
    PUBLIC
    imnodes.h
    imnodes_internal.h
)

# target_include_directories(
#     imnodes
#     PUBLIC
#     ${CMAKE_CURRENT_LIST_DIR}
# )
target_link_libraries(
    imnodes
    PUBLIC
    imgui_sdl2_vulkan
)
