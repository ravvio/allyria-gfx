#define GFX_VERBOSE

#include "../include/gfx/gfx.h"
#include <stdio.h>

int main(void) {
  enum GFX_Graphics_Feature graphics_features[] = {
    GFX_GRAPHICS_FEATURE_ALPHA_BLEND,
  };
  GFX_ProjectSettings settings = {
      .window_width = 800,
      .window_height = 600,
      .window_name = "Engine Basic Test",
      .graphics_features = graphics_features,
      .graphics_features_count =
          sizeof(graphics_features) / sizeof(graphics_features[0]),
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
  ecs_set(world, cam, GFX_CameraPixel, {.ratio = 128.0});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);

  // Set main camera
  engine->main_camera_id = cam;

  GFX_Texture2D atlas =
      gfx_texture2d_load("./assets/textures/sprites/sensei.png",
                         GFX_TEXTURE_FILTER_NEAREST, GFX_WRAP_REPEAT);

  for (int i = 0; i < 10; ++i) {
    ecs_entity_t e = ecs_new(world);
    GFX_Sprite sprite = gfx_sprite_create(&atlas, 0, 0, 16, 23);
    ecs_set_ptr(world, e, GFX_Sprite, &sprite);
    ecs_set(world, e, GFX_SpriteLayer, {0});
    ecs_set(world, e, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
    GFX_Transform *t = ecs_get_mut(world, e, GFX_Transform);
    glm_translate_x(*t, 16.0 * i);
  }

  gfx_run(world);

  gfx_texture2d_destroy(&atlas);
  gfx_terminate(world);
  return 0;
}
