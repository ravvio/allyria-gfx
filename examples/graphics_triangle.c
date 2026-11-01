#include <gfx/gfx.h>
#include <stdio.h>

int main(void) {
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }

  GFX_Window *win = gfx_window_init(800, 600, "Triangle Example");
  if (win == NULL) {
    gfx_graphics_terminate();
    return 1;
  }

  // Define points
  float positions[6] = {
      -0.5, -0.5, // 0
      0.5,  -0.5, // 1
      0.0,  0.6,  // 2
  };
  uint32_t indices[3] = {
      0,
      1,
      2,
  };

  // Triangle buffers
  GFX_VertexBuffer vbo =
      gfx_vertex_buffer_create(3 * 2 * sizeof(float), positions);
  GFX_IndexBuffer ib = gfx_index_buffer_create(3, indices);

  // Triangle layout
  GFX_VertexLayout layout = {0};
  gfx_vertex_layout_push_f32(&layout, 2);

  GFX_Shader shader = gfx_shader_create("./assets/shaders/triangle.wgsl");
  GFX_Pipeline pipeline = gfx_pipeline_create(&shader, &layout, NULL);

  while (!gfx_window_should_close(win)) {
    gfx_frame_begin(win);

    gfx_pipeline_bind(&pipeline);
    gfx_vertex_buffer_bind(&vbo);
    gfx_index_buffer_bind(&ib);
    gfx_draw_indexed(gfx_index_buffer_get_count(&ib));

    gfx_frame_end(win);
    gfx_window_events_poll();
  }

  gfx_pipeline_destroy(&pipeline);
  gfx_shader_destroy(&shader);
  gfx_index_buffer_destroy(&ib);
  gfx_vertex_buffer_destroy(&vbo);
  gfx_window_destroy(win);
  gfx_graphics_terminate();
  return 0;
}
