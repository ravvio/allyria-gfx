#ifndef GFX_SPRITE_DECL
#define GFX_SPRITE_DECL

#include "../gfx_common.h"
#include "../graphics/gfx_graphics.h"

// A rect of a texture atlas, drawn as a quad of its size in pixels centered
// on its GFX_Transform. Sprites need GFX_Sprite, GFX_Transform and
// GFX_SpriteLayer. The texture must outlive the sprite.
typedef struct {
  const GFX_Texture2D *texture;
  // Atlas rect in uv: offset xy, scale zw (top left origin)
  vec4 uv_rect;
  // Quad size in pixels
  vec2 size;
} GFX_Sprite;
extern ECS_COMPONENT_DECLARE(GFX_Sprite);

// Atlas rect in pixels, top left origin
GFX_Sprite gfx_sprite_create(const GFX_Texture2D *texture, int offset_x,
                             int offset_y, int width, int height);

// Draw order, lower layers first. Inside a layer sprites are grouped by
// texture.
typedef uint8_t GFX_SpriteLayer;
extern ECS_COMPONENT_DECLARE(GFX_SpriteLayer);

// Per-instance vertex data of sprite.wgsl (80 bytes)
typedef struct {
  mat4 model;
  vec4 uv_rect;
} GFX_SpriteInstance;

// Renderer singleton, draws all sprites instanced, one draw per run of
// sprites sharing a texture
typedef struct {
  GFX_Shader shader;
  GFX_MaterialLayout material_layout;
  GFX_Pipeline pipeline;
  ecs_query_t *query;

  // Instance vertex buffer, grows when needed
  GFX_VertexBuffer instance_buffer;
  uint32_t instance_capacity;

  // Per frame scratch (stb_ds arrays)
  struct _GFX_SpriteDraw *draws;
  GFX_SpriteInstance *instances;

  // One material per texture view (stb_ds hashmap)
  struct _GFX_SpriteMaterial *materials;
} GFX_SpriteRenderer;
extern ECS_COMPONENT_DECLARE(GFX_SpriteRenderer);

void GfxSpriteImport(ecs_world_t *world);

#endif
