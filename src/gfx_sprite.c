#include <sys/types.h>
#include "../external/cglm/cglm.h"

#include "../include/gfx/gfx.h"

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
  gfx_vertex_layout_push_f32(&layout, 4);

  GFX_VertexArray vao = gfx_vertex_array_create();
  gfx_vertex_array_add_buffer(&vao, &vbo, &layout);

  gfx_vertex_buffer_unbind();
  gfx_vertex_array_unbind();

  GFX_SpriteRenderer renderer = {
    .vao = &vao
  };
  return renderer;
}

void gfx_sprite_renderer_draw(
    GFX_SpriteRenderer *renderer,
    GFX_Sprite *sprite,
    mat4 projection,
    vec3 position
) {
  gfx_shader_bind(sprite->shader);

  mat4 model = GLM_MAT4_IDENTITY;
  glm_translate(model, position);

  gfx_shader_uniform_set_mat4(sprite->shader, "model", 0, model);

  glActiveTexture(GL_TEXTURE0);
  texture.bind();
}
