#include "../include/gfx/gfx_gl.h"

void gfx_sprite_create() {
  GFX_Shader shader =
      gfx_shader_create("./shaders/sprite.vert", "./shaders/sprite.frag");
}

GFX_SpriteRenderer gfx_sprite_renderer_create() {
  float vertices[] = {
      0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
  };
  GFX_VertexBuffer vbo = gfx_vertex_buffer_create(sizeof(vertices), vertices);
  GFX_VertexLayout layout = gfx_vertex_layout_create();
  gfx_vertex_layout_push_f32(&layout, 4, false);

  GFX_VertexArray vao = gfx_vertex_array_create();
  gfx_vertex_array_add_buffer(&vao, &vbo, &layout);

  gfx_vertex_buffer_unbind();
  gfx_vertex_array_unbind();

  GFX_SpriteRenderer renderer = {
    .vao = &vao
  };
  return renderer;
}

// Tells the sprite renderer to render all sprites
void gfx_sprite_renderer_draw(
    GFX_SpriteRenderer *renderer,
    GFX_Sprite *sprite,
    mat4 trasform_projection,
    mat4 trasform_view,
    mat4 trasform_model
) {
  gfx_shader_bind(sprite->shader);

  gfx_shader_uniform_set_mat4(sprite->shader, "t_model", false, trasform_model);

  glActiveTexture(GL_TEXTURE0);
  gfx_texture2d_bind(sprite->texture);
}
