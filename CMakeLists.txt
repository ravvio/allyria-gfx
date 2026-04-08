cmake_minimum_required(VERSION 3.22)
set(NAME allyria-gfx)
project(${NAME} LANGUAGES C)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

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

# Add GLEW
find_package(GLEW REQUIRED)

# Create the GFX library
set(GFXSOURCES
    ./src/gfx_window.c
    ./src/gfx_renderer.c
    ./src/gfx_vertex_buffer.c
    ./src/gfx_index_buffer.c
    ./src/gfx_vertex_array.c
    ./src/gfx_shader.c
)
add_library(gfx ${GFXSOURCES})
target_include_directories(gfx PUBLIC ./src ./include ./include/cglm)
target_link_libraries(gfx PRIVATE OpenGL::GL glfw GLEW::glew cglm_headers)

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
