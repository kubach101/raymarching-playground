cmake_minimum_required(VERSION 4.2)
project(raymarching C)

set(CMAKE_C_STANDARD 17)

find_package(Vulkan REQUIRED)

include(FetchContent)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)

FetchContent_Declare(cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG        v0.9.6
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)  # header-only use; don't build cglm's DLL

FetchContent_MakeAvailable(glfw cglm)

add_executable(raymarching src/main.c src/utils.h src/toolkit.h 
               src/structs.h src/context.h src/window.h src/renderer.h)

target_link_libraries(raymarching PRIVATE Vulkan::Vulkan glfw cglm_headers)

# Vulkan uses a 0..1 depth range (cglm defaults to OpenGL's -1..1)
target_compile_definitions(raymarching PRIVATE CGLM_FORCE_DEPTH_ZERO_TO_ONE)

# --- Shaders: compile GLSL -> SPIR-V at build time with glslc ---
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin REQUIRED)

set(SHADER_SOURCES
    ${CMAKE_SOURCE_DIR}/res/shaders/raymarching.comp)

set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)

set(SPIRV_FILES)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
        COMMAND ${GLSLC} --target-env=vulkan1.3 ${SHADER} -o ${SPIRV}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME}")
    list(APPEND SPIRV_FILES ${SPIRV})
endforeach()

add_custom_target(Shaders DEPENDS ${SPIRV_FILES})
add_dependencies(raymarching Shaders)
target_compile_definitions(raymarching PRIVATE SHADER_DIR="${SHADER_OUTPUT_DIR}/")
