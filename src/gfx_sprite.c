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

// Fuction to setup the sprite renderer
void gfx_sprite_renderer_start_draw(

) {
}

// Tells the sprite renderer to render all sprites
void gfx_sprite_renderer_draw(
    GFX_SpriteRenderer *renderer,
    GFX_Sprite *sprite,
    mat4 projection,
    mat4 view,
    mat4
) {
  gfx_shader_bind(sprite->shader);

  mat4 model = GLM_MAT4_IDENTITY;
  glm_translate(model, position);

  gfx_shader_uniform_set_mat4(sprite->shader, "model", 0, model);

  glActiveTexture(GL_TEXTURE0);
  gfx_texture2d_bind(sprite->texture);
}
