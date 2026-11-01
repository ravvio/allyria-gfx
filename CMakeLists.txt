cmake_minimum_required(VERSION 3.22)
project(allyria-gfx LANGUAGES C CXX) # CXX: Dawn is a C++ library

include(FetchContent)

option(GFX_BUILD_EXAMPLES "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option(GFX_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})

if(PROJECT_IS_TOP_LEVEL)
  set(FETCHCONTENT_QUIET FALSE)
  # compile_commands.json for editors (clangd)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
  endif()
endif()

# Dawn release used for WebGPU, natively (prebuilt) and in the browser
# (emdawnwebgpu port), from github.com/google/dawn/releases
set(DAWN_TAG v20261029.184636)
set(DAWN_REV 2e9d054d9b5b9df3407057192e7936e2fb41c816)

# glfw3webgpu (below) links the 'glfw' and 'webgpu' targets
add_library(webgpu INTERFACE)

if(EMSCRIPTEN)
  # Browser: GLFW 3.4 API from Emscripten's contrib.glfw3 port (the built-in
  # -sUSE_GLFW=3 is GLFW 3.2, no glfwGetError), WebGPU from Dawn's emdawnwebgpu
  add_library(glfw INTERFACE)
  target_compile_options(glfw INTERFACE --use-port=contrib.glfw3)
  target_link_options(glfw INTERFACE --use-port=contrib.glfw3)

  set(EMDAWNWEBGPU_PORT ${CMAKE_CURRENT_BINARY_DIR}/emdawnwebgpu-${DAWN_TAG}.remoteport.py)
  file(DOWNLOAD
      "https://github.com/google/dawn/releases/download/${DAWN_TAG}/emdawnwebgpu-${DAWN_TAG}.remoteport.py"
      ${EMDAWNWEBGPU_PORT}
      EXPECTED_HASH SHA256=6ae253a111e30c556f3ff95be9c26617a24e7cfc708752a79b8a33ef09e1ad1b
  )
  target_compile_options(webgpu INTERFACE --use-port=${EMDAWNWEBGPU_PORT})
  # ASYNCIFY lets init wait on the adapter/device requests (wgpuInstanceWaitAny),
  # DEFAULT_TO_CXX links with the C++ runtime emdawnwebgpu needs
  target_link_options(webgpu INTERFACE --use-port=${EMDAWNWEBGPU_PORT}
                      -sASYNCIFY -sDEFAULT_TO_CXX)
  target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_EMDAWNWEBGPU)
else()

# Add GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw"
    GIT_TAG "3.4"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# Add Dawn (WebGPU) prebuilt
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
target_link_libraries(webgpu INTERFACE dawn::webgpu_dawn)

endif()

# Add glfw3webgpu (WebGPU surface from a GLFW window)
FetchContent_Declare(
    glfw3webgpu
    GIT_REPOSITORY "https://github.com/eliemichel/glfw3webgpu"
    GIT_TAG "v1.3.0-alpha"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw3webgpu)

# Vendored libraries, each exposes only its own headers

# flecs, header and source from the same release
add_library(flecs ${CMAKE_CURRENT_SOURCE_DIR}/vendor/flecs/flecs.c)
target_include_directories(flecs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor/flecs)
if(EMSCRIPTEN)
  # flecs build docs for Emscripten: a 1MB stack (ecs_init overflows the
  # default 64KB) and a growable heap
  target_link_options(flecs INTERFACE -sSTACK_SIZE=1mb -sALLOW_MEMORY_GROWTH=1)
endif()

# cglm and stb, header only
add_library(cglm INTERFACE)
target_include_directories(cglm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cglm/include)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb)

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

target_compile_features(gfx PRIVATE c_std_23)
set_target_properties(gfx PROPERTIES C_EXTENSIONS OFF)

# Consumers include <gfx/...>, webgpu.h, flecs and cglm are part of the public
# API
target_include_directories(gfx
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(gfx PUBLIC webgpu flecs cglm PRIVATE glfw glfw3webgpu stb)

if(GFX_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Tests, run with: ctest --test-dir build
if(GFX_BUILD_TESTS AND NOT EMSCRIPTEN)
  enable_testing()
  add_executable(test_texture_mips tests/test_texture_mips.c)
  # Tests check library internals
  target_include_directories(test_texture_mips PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(test_texture_mips PRIVATE glfw gfx)
  add_test(NAME texture_mips COMMAND test_texture_mips)
endif()
