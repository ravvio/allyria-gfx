# Examples, built the way a game uses the engine: they link gfx and ship their
# own assets
project(allyria-gfx-examples LANGUAGES C)

# Assets are loaded from ./assets, next to the executables
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

function(add_example NAME)
  add_executable(${NAME} ${NAME}.c)
  target_compile_features(${NAME} PRIVATE c_std_23)
  set_target_properties(${NAME} PROPERTIES C_EXTENSIONS OFF)
  if(MSVC)
    target_compile_options(${NAME} PRIVATE /W4)
  else()
    target_compile_options(${NAME} PRIVATE -Wall -Wextra -pedantic)
  endif()
  target_link_libraries(${NAME} PRIVATE gfx)

  # Browser: an .html page with the assets packaged at /assets
  if(EMSCRIPTEN)
    set_target_properties(${NAME} PROPERTIES SUFFIX ".html")
    target_link_options(${NAME} PRIVATE
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets")
  endif()
endfunction()

# The graphics_* examples run their own blocking loop, native only
if(NOT EMSCRIPTEN)
  add_example(graphics_basic)
  add_example(graphics_triangle)
  add_example(graphics_geometry2d)
  add_example(graphics_geometry3d)
  add_example(graphics_material_unlit)
  # The browser has one window
  add_example(engine_windows)
endif()

add_example(engine_basic)
add_example(engine_sprites)
add_example(engine_viewports)
add_example(engine_render_target)
add_example(engine_perspective_camera)
