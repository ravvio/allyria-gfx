#include "../include/gfx/gfx.h"

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
      .clear_color = { 0.2, 0.3, 0.4, 1.0 },
  };
  ecs_world_t *world = ecs_init();

  if (gfx_init(&settings, world) != GFX_ERR_OK) {
    gfx_terminate();
    return 1;
  }

  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);

  while (ecs_progress(world, 0)) {
  }

  gfx_terminate();
  return 0;
}
