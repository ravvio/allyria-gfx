#define GFX_VERBOSE

#include "../include/gfx/gfx.h"
#include <stdio.h>

int main(void) {
  enum GFX_Graphics_Feature graphics_features[] = {
      GFX_GRAPHICS_FEATURE_DEPTH_TESTING,
      GFX_GRAPHICS_FEATURE_CULL_BACK,
  };
  GFX_ProjectSettings settings = {
      .window_width = 800,
      .window_height = 600,
      .window_name = "Engine Basic Test",
      .graphics_features = graphics_features,
      .graphics_features_count = 2,
      .clear_color = {0.2, 0.3, 0.4, 1.0},
  };
  ecs_world_t *world = ecs_init();

  if (gfx_init(&settings, world) != GFX_ERR_OK) {
    gfx_terminate(world);
    return 1;
  }

  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);
  ECS_IMPORT(world, GfxSprite);

  GFX_Engine *engine = ecs_singleton_get_mut(world, GFX_Engine);

  ecs_entity_t cam = ecs_entity(world, {.name = "Camera"});
  ecs_set(world, cam, GFX_Camera,
          {
              .projection = GLM_MAT4_IDENTITY_INIT,
              .view = GLM_MAT4_IDENTITY_INIT,
              .model = GLM_MAT4_IDENTITY_INIT,
          });
  ecs_set(world, cam, GFX_CameraOrtho, {.aspect = 800.0 / 600.0});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);

  // Set main camera
  engine->main_camera_id = cam;

  GFX_Texture2D atlas = gfx_texture2d_load(
      "./assets/textures/sprites/sensei.png", GFX_TEXTURE_TYPE_DIFFUSE,
      GFX_TEXTURE_FILTER_NEAREST, GFX_WRAP_REPEAT);
  GFX_Sprite sprite = gfx_sprite_create(&atlas, 0, 0, 16, 23, 0);

  ecs_entity_t e = ecs_entity(world, {.name = "Sensei"});
  ecs_set(world, e, GFX_Sprite,
          {.quad = sprite.quad, .texture_atlas = sprite.texture_atlas});
  ecs_set(world, e, GFX_SpriteLayer, {0});

  while (ecs_progress(world, 0)) {
  }

  gfx_terminate(world);
  return 0;
}
