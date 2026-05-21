cmake_minimum_required(VERSION 3.10)
project(3D-Viewer)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_CXX_STANDARD 17)

set(TARGET_NAME 3D-Viewer)
set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Externals")

# enable SIMD
add_definitions("-DVIEWER_SIMD_OPT")
add_definitions(-DASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets/")

# debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-DDEBUG")
endif ()

find_program(CCACHE "ccache")
if (CCACHE)
    message(STATUS "find ccache")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif (CCACHE)

set(SHADER_LIBS_RELEASE 
"${THIRD_PARTY_DIR}/shaderc/lib/shaderc_combined.lib"
"${THIRD_PARTY_DIR}/spirv_cross/lib/spirv-cross-core.lib"
"${THIRD_PARTY_DIR}/spirv_cross/lib/spirv-cross-cpp.lib"
"${THIRD_PARTY_DIR}/spirv_cross/lib/spirv-cross-glsl.lib"
)
set(SHADER_LIBS_DEBUG 
"${THIRD_PARTY_DIR}/shaderc/lib/shaderc_combinedd.lib"
"${THIRD_PARTY_DIR}/spirv_cross/lib/spirv-cross-cored.lib"
"${THIRD_PARTY_DIR}/spirv_cross/lib/spirv-cross-cppd.lib"
"${THIRD_PARTY_DIR}/spirv_cross/lib/spirv-cross-glsld.lib"
)

include_directories(
        "${THIRD_PARTY_DIR}/glfw/include"
        "${THIRD_PARTY_DIR}/glad2/include"
        "${THIRD_PARTY_DIR}/glm"
        "${THIRD_PARTY_DIR}/assimp/include"
        "${THIRD_PARTY_DIR}/assimp/contrib"
        "${THIRD_PARTY_DIR}/imgui"
        "${THIRD_PARTY_DIR}/json11"
        "${THIRD_PARTY_DIR}/stb_image"
        "${THIRD_PARTY_DIR}/material_symbols"
        "${THIRD_PARTY_DIR}/vulkan/include"
        "${THIRD_PARTY_DIR}/shaderc/include"
        "${THIRD_PARTY_DIR}/spirv_cross/include"
        "${THIRD_PARTY_DIR}/vma"
        "${THIRD_PARTY_DIR}/renderdoc"
        "${CMAKE_CURRENT_SOURCE_DIR}/Sources"
)

# assimp
set(BUILD_SHARED_LIBS OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_ZLIB ON)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE)

add_subdirectory(${THIRD_PARTY_DIR}/assimp)
add_subdirectory(${THIRD_PARTY_DIR}/glfw)
add_subdirectory(${THIRD_PARTY_DIR}/spdlog)
add_subdirectory(${THIRD_PARTY_DIR}/imgui)

# glslang
set(SKIP_GLSLANG_INSTALL ON)
set(ENABLE_SPVREMAPPER OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_CTEST OFF)
set(ENABLE_PCH OFF)
set(BUILD_TESTING OFF)
set(USE_CCACHE ON)
add_subdirectory("${THIRD_PARTY_DIR}/glslang")

# main src
file(GLOB VIEWER_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/Sources/Core/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Sources/Core/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Sources/Renderer/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Sources/Renderer/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Sources/main.cpp
        )

add_executable(${TARGET_NAME}
        "${VIEWER_SRC}"
        "${THIRD_PARTY_DIR}/glad2/src/gl.c"
        "${THIRD_PARTY_DIR}/json11/json11.cpp"
        "${THIRD_PARTY_DIR}/stb_image/stb_image.cpp"
         "Sources/Renderer/Buffer.cpp")

set(LINK_LIBS
        assimp
        #glslang
        #glslang-default-resource-limits
        #SPIRV
        )

if (WIN32)
    find_package(OpenGL REQUIRED)
endif ()

if (APPLE)

endif ()

if (UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
endif ()

if (MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /std:c++17 /Zc:__cplusplus>)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mfma -O3")
endif ()

target_link_libraries(${TARGET_NAME} ${LINK_LIBS} glfw spdlog imgui "$<$<CONFIG:Debug>:${SHADER_LIBS_DEBUG}>"
    "$<$<CONFIG:Release>:${SHADER_LIBS_RELEASE}>")

# output dir
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# copy assets
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${TARGET_NAME}>/assets

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/GLSL $<TARGET_FILE_DIR:${TARGET_NAME}>/shaders/GLSL

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets/Shaders/Cache $<TARGET_FILE_DIR:${TARGET_NAME}>/Cache

        COMMAND ${CMAKE_COMMAND} -E copy
        ${THIRD_PARTY_DIR}/renderdoc/renderdoc.dll $<TARGET_FILE_DIR:${TARGET_NAME}>/renderdoc.dll
        )
