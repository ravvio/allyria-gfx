#ifndef GFX_SPRITE_DECL
#define GFX_SPRITE_DECL

#include "../gfx_common.h"
#include "../graphics/gfx_graphics.h"

#define GFX_SPRITE_LAYERS_COUNT 16

typedef struct {
  GFX_Geometry2D quad;
  GFX_Texture2D *texture_atlas;
} GFX_Sprite;
extern ECS_COMPONENT_DECLARE(GFX_Sprite);

GFX_Sprite gfx_sprite_create(GFX_Texture2D *texture_atlas, int offset_x, int offset_y,
                       int width, int height, u_int8_t layer);

typedef u_int8_t GFX_SpriteLayer;
extern ECS_COMPONENT_DECLARE(GFX_SpriteLayer);

typedef struct {
  GFX_Shader shader;
  GFX_Sprite **layers[GFX_SPRITE_LAYERS_COUNT];
} GFX_SpriteRenderer;
extern ECS_COMPONENT_DECLARE(GFX_SpriteRenderer);

void gfx_sprite_renderer_add_sprite(GFX_SpriteRenderer *sr, GFX_Sprite *sprite, GFX_SpriteLayer layer);
void gfx_sprite_renderer_clear(GFX_SpriteRenderer *sr);

void GfxSpriteImport(ecs_world_t *world);

#endif
