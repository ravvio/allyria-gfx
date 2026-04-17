#include "../include/gfx/gfx_gl.h"

void gfx_sprite_create() {
}

GFX_SpriteRenderer gfx_sprite_renderer_create() {
  // Load the sprite shader
  GFX_Shader shader =
      gfx_shader_create("./shaders/sprite.vert", "./shaders/sprite.frag");

  // We use a single mesh for all the sprites
  GFX_VertexLayout sprite_vertex_layout = gfx_vertex_layout_create();

  // Create renderer
  GFX_SpriteRenderer renderer = {
    .shader = &shader,
    .layers = {},
  };

  // Initialize layers
  for (int i = 0; i < SPRITE_LAYERS_COUNT; ++i) {
    GFX_Sprite* sprites = ecs_os_malloc(0);
    GFX_SpriteLayer layer = {
      .sprites = sprites,
      .sprites_count = 0,
    };
    renderer.layers[i] = layer;
  }

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
  gfx_shader_bind(renderer->shader);

  for (int i = SPRITE_LAYERS_COUNT - 1; i >= 0; --i) {
    GFX_SpriteLayer layer = renderer->layers[i];
  }

  glActiveTexture(GL_TEXTURE0);
  gfx_texture2d_bind(sprite->texture);
}
