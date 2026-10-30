cmake_minimum_required(VERSION 3.22)
set(NAME allyria-gfx)
project(${NAME} LANGUAGES C CXX) # CXX: Dawn is a C++ library

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# compile_commands.json for editors (clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Architecture string.
set(ARCH "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "Override CPU architecture")
string(REGEX REPLACE "amd64" "x86_64" KERNEL_ARCH "${ARCH}")

# Set directory locations (allowing us to move directories easily)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${ARCH})

# Add GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw"
    GIT_TAG "3.4"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# Add Dawn (WebGPU), prebuilt nightly from github.com/google/dawn/releases
# ponytail: Release archives only, Windows Debug builds need Dawn's Debug archive (MSVC runtime)
set(DAWN_TAG v20261029.184636)
set(DAWN_REV 2e9d054d9b5b9df3407057192e7936e2fb41c816)
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(DAWN_PLATFORM macos-15-intel)
  set(DAWN_SHA256 a24900577a4ba760717af0f745d80799a0d2fabaca488813e6ba79cf42bce4a5)
elseif(APPLE)
  set(DAWN_PLATFORM macos-latest)
  set(DAWN_SHA256 28ebb58fc9c787de5bb0fef7248186961b9186fad98b78586088961603a427d0)
elseif(WIN32)
  set(DAWN_PLATFORM windows-latest)
  set(DAWN_SHA256 e84d650385a7d5e8583e0ae793fa222d19c08cfe822a3c252d8de40ad467bf72)
else()
  set(DAWN_PLATFORM ubuntu-latest)
  set(DAWN_SHA256 86338ce8fda01f6583a660de341bde5b72c8ebd9e39f05513f305f63408554e9)
endif()
FetchContent_Declare(
    dawn
    URL "https://github.com/google/dawn/releases/download/${DAWN_TAG}/Dawn-${DAWN_REV}-${DAWN_PLATFORM}-Release.tar.gz"
    URL_HASH SHA256=${DAWN_SHA256}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(dawn)
find_package(Threads REQUIRED)
find_package(Dawn CONFIG REQUIRED PATHS ${dawn_SOURCE_DIR} NO_DEFAULT_PATH)

# Add glfw3webgpu (WebGPU surface from a GLFW window), it links a 'webgpu' target
add_library(webgpu INTERFACE)
target_link_libraries(webgpu INTERFACE dawn::webgpu_dawn)
FetchContent_Declare(
    glfw3webgpu
    GIT_REPOSITORY "https://github.com/eliemichel/glfw3webgpu"
    GIT_TAG "v1.3.0-alpha"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw3webgpu)

# Add cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY "https://github.com/recp/cglm"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cglm)

# Add flecs (vendored, so the library matches include/flecs/flecs.h)
add_library(flecs ./include/flecs/flecs.c)

# Create the GFX library
set(GFXSOURCES
    ./src/graphics/gfx_glfw.c
    ./src/graphics/gfx_window.c
    ./src/graphics/gfx_buffers.c
    ./src/graphics/gfx_shader.c
    ./src/graphics/gfx_uniforms.c
    ./src/graphics/gfx_geometry2d.c
    ./src/graphics/gfx_geometry3d.c
    ./src/graphics/gfx_texture.c
    ./src/graphics/gfx_material.c

    ./src/renderer/gfx_renderer.c
    ./src/renderer/gfx_viewport.c
    ./src/renderer/gfx_surface.c

    ./src/camera/gfx_camera.c

    ./src/sprite/gfx_sprite.c

    ./src/gfx.c
    ./src/gfx_engine.c
    ./src/gfx_transform.c
)
add_library(gfx ${GFXSOURCES})
# webgpu.h is part of the public graphics API
target_link_libraries(gfx PUBLIC dawn::webgpu_dawn PRIVATE glfw glfw3webgpu cglm_headers flecs)

# Copy full directories
macro(copy_resource_dirs dirs)
    foreach(dir ${dirs})
        # Replace / at the end of the path (copy dir content VS copy dir)
        string(REGEX REPLACE "/+$" "" dirclean "${dir}")
        message(STATUS "Copying resource ${dirclean} to ${BUILD_DIR}")
        file(COPY ${dirclean} DESTINATION ${BUILD_DIR})
    endforeach()
endmacro()
copy_resource_dirs("${CMAKE_SOURCE_DIR}/assets")

# Add examples
function(add_exe EXE_NAME)
    set(EXE_SOURCE "examples/${EXE_NAME}.c")
    set(EXE_SOURCES ${COMMON_HEADERS} ${COMMON_SOURCES} ${EXE_SOURCE})

    add_executable(${EXE_NAME} ${EXE_SOURCE})
    set_target_properties(${EXE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        INTRAPROCEDURAL_OPTIMIZATION ON
    )

    # Target compilation options
    if(MSVC)
        target_compile_options(${EXE_NAME} PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
        target_compile_options(${EXE_NAME} PRIVATE /fp:fast)
    else()
        target_compile_options(${EXE_NAME} PRIVATE -Wall -Wextra -pedantic)
        target_compile_options(${EXE_NAME} PRIVATE -ffast-math)
    endif()

    # Include directories
    target_include_directories(${EXE_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )

    # And libraries
    target_link_libraries(${EXE_NAME} PRIVATE cglm_headers flecs gfx)
endfunction()

add_exe(graphics_basic)
add_exe(graphics_triangle)
add_exe(graphics_geometry2d)
add_exe(graphics_geometry3d)
add_exe(graphics_material_std_unlit)

add_exe(engine_basic)
add_exe(engine_sprites)

# Tests, run with: ctest --test-dir build
enable_testing()
add_executable(test_texture_mips tests/test_texture_mips.c)
target_link_libraries(test_texture_mips PRIVATE glfw gfx)
add_test(NAME texture_mips COMMAND test_texture_mips)
