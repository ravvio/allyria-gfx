cmake_minimum_required(VERSION 3.22)
set(NAME allyria-gfx)
project(${NAME} LANGUAGES C)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Architecture string.
set(ARCH "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "Override CPU architecture")
string(REGEX REPLACE "amd64" "x86_64" KERNEL_ARCH "${ARCH}")

# Set directory locations (allowing us to move directories easily)
set(BUILD_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${ARCH})

# Add OpenGL
find_package(OpenGL REQUIRED)

# Add GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw"
    GIT_TAG "3.4"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# Add glad
add_library(glad ./external/gl.c)

# Add cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY "https://github.com/recp/cglm"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cglm)

# Add flecs
FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG master
)
FetchContent_MakeAvailable(flecs)

# Create the GFX library
set(GFXSOURCES
    ./src/gfx_gl.c
    ./src/gfx_glfw.c
    ./src/gfx_window.c
    ./src/gfx_events.c
    ./src/gfx_buffers.c
    ./src/gfx_shader.c
    ./src/gfx_texture.c
    ./src/gfx_mesh2d.c
    ./src/gfx_mesh3d.c

    ./src/gfx_transform.c

    ./src/gfx_renderer.c
    ./src/gfx_viewport.c

    ./src/gfx_camera.c
)
add_library(gfx ${GFXSOURCES})
target_include_directories(gfx PUBLIC ./src ./external)
target_link_libraries(gfx PRIVATE OpenGL::GL glfw glad cglm_headers flecs::flecs_static)

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
    target_link_libraries(${EXE_NAME} PRIVATE OpenGL::GL glfw glad cglm_headers flecs::flecs_static gfx)
endfunction()

add_exe(basic)
add_exe(triangle)
add_exe(mesh2d)
add_exe(mesh3d)
add_exe(solid3d)
add_exe(material_std_unlit)
