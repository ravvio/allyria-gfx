#include "../include/gfx/gfx.h"

int main(void) {
  if (gfx_init() != GFX_ERR_OK) {
    gfx_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Triangle Example");

  // Define points
  float positions[6] = {
      -0.5, -0.5, // 0
      0.5,  -0.5, // 1
      0.0,  0.6,  // 2
  };
  u_int32_t indices[3] = {
      0,
      1,
      2,
  };

  // Triangle vbo
  GFX_VertexBuffer vbo =
      gfx_vertex_buffer_create(3 * 2 * sizeof(float), positions);

  // Triangle vao
  GFX_VertexArray vao = gfx_vertex_array_create();

  // Triangle vao layout
  GFX_VertexLayout layout = gfx_vertex_layout_create();
  gfx_vertex_layout_push_f32(&layout, 2, false);

  // Triangle index buffer
  GFX_IndexBuffer ib = gfx_index_buffer_create(6, indices);

  gfx_vertex_array_add_buffer(&vao, &vbo, &layout);

  GFX_Shader shader = gfx_shader_create("./assets/shaders/shape_2d.vert",
                                        "./assets/shaders/shape_2d.frag");

  while (!gfx_window_should_close(win)) {
    gfx_buffers_clear();

    gfx_shader_bind(&shader);
    gfx_vertex_array_bind(&vao);
    gfx_index_buffer_bind(&ib);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, NULL);

    gfx_window_swap_buffers(win);
    gfx_events_poll();
  }

  gfx_terminate();
  return 0;
}
