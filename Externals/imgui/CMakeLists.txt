cmake_minimum_required (VERSION 3.16)

project(imgui)

set(IMGUI_SRC_FILES
    "imconfig.h"
    "imgui.cpp"
    "imgui.h"
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_internal.h"
    "imgui_tables.cpp"
    "imgui_widgets.cpp"
    "imstb_rectpack.h"
    "imstb_textedit.h"
    "imstb_truetype.h"
    "backends/imgui_impl_glfw.h"
    "backends/imgui_impl_glfw.cpp"
    "backends/imgui_impl_opengl3.h"
    "backends/imgui_impl_opengl3.cpp"
)

add_library(imgui ${IMGUI_SRC_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${IMGUI_SRC_FILES} )

set_target_properties(imgui PROPERTIES CXX_STANDARD 17)
