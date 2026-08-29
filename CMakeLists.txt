cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(WIN32)
        # Keep the hardcoded path used on the original Windows machine.
        set(CMAKE_TOOLCHAIN_FILE "D:/Codes/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    elseif(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    endif()
endif()

project(glTFViewer C CXX)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GLFW3, GLEW, GLM, ImGui, httplib, and nlohmann_json
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# Config file is named KtxConfig.cmake (case-sensitive on Linux).
find_package(Ktx CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(ext/ImGuiFileDialog)

add_executable(glTF_2_0_Viewer
"main.cpp"
"model.cpp"
"camera.h"
"backend.hpp"
"logger.h"
"shaderprogram.h"
"buffer.hpp"
"model.h"
"application.h"
"application.cpp"
"utils.hpp"
"gui.h"
"gui.cpp"
"renderer.h"
"renderer.cpp"
"render_pass.h"
"pipeline.hpp"
"descriptor.hpp"
"texture.hpp"
"framebuffer.hpp"
"viewer_app.h"
"viewer_app.cpp"
"stb_impl.cpp"
)

target_link_libraries(glTF_2_0_Viewer PRIVATE
    glfw
    GLEW::GLEW
    glm::glm
    imgui::imgui
    httplib::httplib
    nlohmann_json::nlohmann_json
    ImGuiFileDialog
    KTX::ktx
    OpenSSL::SSL
)

# Expose the vcpkg installed include directory so headers without a CMake
# config (tinygltf's tiny_gltf.h, stb's stb_image.h, etc.) are found.
if(DEFINED VCPKG_INSTALLED_DIR)
    target_include_directories(glTF_2_0_Viewer PRIVATE "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
endif()

# Executable 2: GPU-Driven Meshlet Renderer (MDI)
add_executable(gpu_driven gpu_driven.cpp "camera.h" "backend.hpp")
target_link_libraries(gpu_driven PRIVATE
    glfw
    GLEW::GLEW
    assimp::assimp
    glm::glm
    imgui::imgui
)

# Post-build copy rules for glTF_2_0_Viewer assets
add_custom_command(TARGET glTF_2_0_Viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Bee.glb"
        "$<TARGET_FILE_DIR:glTF_2_0_Viewer>/Bee.glb"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/vert.spv"
        "$<TARGET_FILE_DIR:glTF_2_0_Viewer>/vert.spv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/frag.spv"
        "$<TARGET_FILE_DIR:glTF_2_0_Viewer>/frag.spv"
)

# Post-build copy rules for gpu_driven assets
add_custom_command(TARGET gpu_driven POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Bee.glb"
        "$<TARGET_FILE_DIR:gpu_driven>/Bee.glb"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cull_comp.spv"
        "$<TARGET_FILE_DIR:gpu_driven>/cull_comp.spv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/meshlet_vert.spv"
        "$<TARGET_FILE_DIR:gpu_driven>/meshlet_vert.spv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/meshlet_frag.spv"
        "$<TARGET_FILE_DIR:gpu_driven>/meshlet_frag.spv"
)
