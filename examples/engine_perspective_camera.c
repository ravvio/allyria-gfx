#include <gfx/gfx.h>
#include <math.h>

// Swing the camera around the sprites, 300 units away
static void orbit(ecs_iter_t *it) {
  GFX_Transform *t = ecs_field(it, GFX_Transform, 0);
  float angle = 0.4f * sinf(ecs_get_world_info(it->world)->world_time_total);
  for (int i = 0; i < it->count; ++i) {
    glm_rotate_make(t[i], angle, (vec3){0.0f, 1.0f, 0.0f});
    glm_translate(t[i], (vec3){0.0f, 0.0f, 300.0f});
  }
}

int main(void) {
  enum GFX_GraphicsFeature graphics_features[] = {
      GFX_GRAPHICS_FEATURE_ALPHA_BLEND,
  };
  GFX_ProjectSettings settings = {
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
  ecs_entity_t window = ecs_new(world);
  ecs_set(world, window, GFX_Surface,
          {.title = "Engine Perspective Camera", .width = 800, .height = 600});

  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);
  ECS_IMPORT(world, GfxSprite);

  ECS_TAG(world, Orbiting);
  ECS_SYSTEM(world, orbit, EcsOnUpdate, GFX_Transform, Orbiting);

  ecs_entity_t cam = ecs_new(world);
  ecs_set(world, cam, GFX_Camera,
          {
              .projection = GLM_MAT4_IDENTITY_INIT,
              .view = GLM_MAT4_IDENTITY_INIT,
          });
  ecs_set(world, cam, GFX_CameraPerspective,
          {.fov = glm_rad(60.0f), .near = 1.0f, .far = 2000.0f});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
  ecs_add(world, cam, Orbiting);

  ecs_entity_t view = ecs_new(world);
  ecs_set(world, view, GFX_Viewport,
          {.camera = cam, .surface = window, .rect = {0, 0, 1, 1}});
  ecs_add(world, ecs_new_w_pair(world, EcsChildOf, view), GFX_SpriteBatch);

  GFX_Texture2D atlas =
      gfx_texture2d_load("./assets/textures/sprites/sensei.png",
                         GFX_TEXTURE_FILTER_NEAREST, GFX_WRAP_REPEAT);

  // A row going away from the camera, farther sprites on lower layers so
  // nearer ones draw on top
  for (int i = 0; i < 10; ++i) {
    ecs_entity_t e = ecs_new(world);
    GFX_Sprite sprite = gfx_sprite_create(&atlas, 0, 0, 16, 23);
    ecs_set_ptr(world, e, GFX_Sprite, &sprite);
    ecs_set(world, e, GFX_SpriteLayer, {9 - i});
    ecs_set(world, e, GFX_Transform, GLM_MAT4_IDENTITY_INIT);
    GFX_Transform *t = ecs_get_mut(world, e, GFX_Transform);
    glm_translate(*t, (vec3){24.0f * (i - 4.5f), 0.0f, -80.0f * i});
  }

  gfx_run(world);

  gfx_texture2d_destroy(&atlas);
  gfx_terminate(world);
  return 0;
}
