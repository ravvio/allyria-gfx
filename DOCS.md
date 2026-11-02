# Architecture

How the engine is put together, and the rules that keep it working. Build
instructions are in `README.md`, contributor conventions in `AGENTS.md`, the
references behind the design in `SOURCES.md`.

## Layers

```
game                 examples/*.c, links `gfx`, includes <gfx/gfx.h>
  │
ECS modules          GfxEngine, GfxRenderer (windows, phases, viewports),
  │                  GfxTransform, GfxCamera, GfxSprite
  │                  use only the low-level API, never wgpu* directly
low-level API        src/graphics/: device, windows, frames and views,
  │                  buffers, shaders, pipelines, uniforms, materials,
  │                  textures, geometry
WebGPU + GLFW        Dawn natively, emdawnwebgpu in the browser
```

WebGPU state (device, queue, the frame's encoder and pass) lives in the `_gfx`
struct, `src/gfx_internal.h`.

## A minimal game

```c
#include <gfx/gfx.h>

int main(void) {
  GFX_ProjectSettings settings = {.clear_color = {0.2, 0.3, 0.4, 1.0}};
  ecs_world_t *world = ecs_init();
  if (gfx_init(&settings, world) != GFX_ERR_OK) {
    gfx_terminate(world);
    return 1;
  }
  ECS_IMPORT(world, GfxTransform);
  ECS_IMPORT(world, GfxCamera);
  ECS_IMPORT(world, GfxSprite);

  // A window
  ecs_entity_t window = ecs_new(world);
  ecs_set(world, window, GFX_Surface,
          {.title = "Game", .width = 800, .height = 600});

  // A camera showing 128 world units above and below its center
  ecs_entity_t cam = ecs_new(world);
  ecs_set(world, cam, GFX_Camera,
          {.projection = GLM_MAT4_IDENTITY_INIT, .view = GLM_MAT4_IDENTITY_INIT});
  ecs_set(world, cam, GFX_CameraOrtho, {.size = 128});
  ecs_set(world, cam, GFX_Transform, GLM_MAT4_IDENTITY_INIT);

  // The camera in the whole window, with a sprite renderer
  ecs_entity_t view = ecs_new(world);
  ecs_set(world, view, GFX_Viewport,
          {.camera = cam, .surface = window, .rect = {0, 0, 1, 1}});
  ecs_add(world, ecs_new_w_pair(world, EcsChildOf, view), GFX_SpriteBatch);

  // A sprite
  GFX_Texture2D atlas = gfx_texture2d_load("./assets/sprites.png",
                                           GFX_TEXTURE_FILTER_NEAREST,
                                           GFX_WRAP_REPEAT);
  ecs_entity_t e = ecs_new(world);
  GFX_Sprite sprite = gfx_sprite_create(&atlas, 0, 0, 16, 16);
  ecs_set_ptr(world, e, GFX_Sprite, &sprite);
  ecs_set(world, e, GFX_SpriteLayer, {0});
  ecs_set(world, e, GFX_Transform, GLM_MAT4_IDENTITY_INIT);

  gfx_run(world); // returns when every window is closed (never in the browser)

  gfx_texture2d_destroy(&atlas);
  gfx_terminate(world);
  return 0;
}
```

## Frame

`gfx_run` calls `ecs_progress` every frame. The phases, in order:

| Phase | What runs |
|---|---|
| `EcsOnLoad` | each `GFX_Surface` reads its window's size |
| `EcsPreUpdate` | closed windows are deleted; quit when none is left |
| `EcsOnUpdate`, `EcsPostUpdate` | gameplay |
| `GfxPreRender` | cameras: `view` = inverse of `GFX_Transform`, square `projection` |
| `GfxStartRender` | `gfx_frame_begin`, then every window is acquired and cleared |
| `GfxRenderObjects` | the viewport loop (below) |
| `GfxRenderGizmos`, `GfxRenderGUI` | empty for now |
| `GfxEndRender` | `gfx_frame_end`: upload matrices, one submit, present every window |
| `GfxPostRender` | poll window events |

Systems that depend on each other go in different phases: flecs doesn't order
systems inside a phase.

## Windows

A window is an entity with `GFX_Surface`. Setting the component opens the
window (`src/renderer/gfx_surface.c`); setting it again keeps the window.

- `width`/`height` are the window size in screen coordinates,
  `true_width`/`true_height` the same divided by the content scale. Both are
  updated every frame; viewports only use their ratio.
- Every window uses the first window's pixel format, so one pipeline draws in
  all of them.
- Closing a window deletes its entity. The app quits when no window is left.
- The browser has one window.

## Viewports and render batches

A viewport is an entity with `GFX_Viewport`: a camera, where it draws (a
window's `surface`, or a render `texture`), a `rect` in fractions of the
target from the top left, and an `order`. Its children with
`GFX_RenderBatch` are its renderers, drawn in the order they were added.

The viewport loop (`src/renderer/gfx_viewport.c`), once per frame:

1. viewports drawing into textures first, then by `order` (higher on top);
2. view-projection = aspect fit × camera projection × camera view;
3. `gfx_frame_view_begin(window, rect, vp)` or
   `gfx_frame_view_begin_texture(texture, rect, vp)`;
4. `draw` of each render batch.

Draw code belongs in a render batch, not in a system: systems run once per
frame, batches once per viewport. A batch of your own:

```c
typedef struct {
  GFX_Geometry3D geometry;
  GFX_Material material;
  GFX_Pipeline pipeline;
  mat4 model;
} Cube;
ECS_COMPONENT_DECLARE(Cube);

static void cube_draw(ecs_world_t *world, ecs_entity_t batch,
                      const GFX_Viewport *viewport) {
  (void)viewport;
  Cube *cube = ecs_get_mut(world, batch, Cube);
  gfx_pipeline_bind(&cube->pipeline);
  gfx_material_bind(&cube->material);
  gfx_draw_set_model(cube->model);
  gfx_geometry3d_draw(&cube->geometry);
}

// After the window opened (the pipeline needs its format)
ecs_entity_t batch = ecs_new_w_pair(world, EcsChildOf, view);
ecs_set_ptr(world, batch, Cube, &cube);
ecs_set(world, batch, GFX_RenderBatch, {.draw = cube_draw});
```

Every view gets its own render pass with a cleared depth buffer, so
overlapping viewports don't share depth.

## Cameras

`GFX_Camera` holds `projection` and `view`, plus a `scaling_mode`. Systems fill
them from other components:

- `GFX_Transform` → `view` (its inverse: the world moves opposite to the
  camera);
- `GFX_CameraOrtho {size}` → orthographic projection, `size` = half the visible
  extent along the fixed axis, in world units (pixels for sprites);
- `GFX_CameraPerspective {fov, near, far}` → perspective projection, `fov`
  along the fixed axis, in radians. The camera looks down −Z.

Both projections are built for a square view. The viewport stretches them to
its own shape (`gfx_camera_view_projection`, `src/camera/gfx_camera.c`):

- `GFX_CAMERA_SCALING_MODE_FIT_HEIGHT`: x is scaled by `1 / aspect`, the
  height stays fixed;
- `GFX_CAMERA_SCALING_MODE_FIT_WIDTH`: y is scaled by `aspect`, the width stays
  fixed.

That's exact for both projections (`tests/test_viewport.c` checks it), and one
camera can be shown in viewports of different shapes.

## Render textures

A `GFX_RenderTexture` is a texture plus a depth buffer that viewports can draw
into. The game owns it, like any texture.

```c
GFX_RenderTexture screen =
    gfx_render_texture_create(160, 120, GFX_TEXTURE_FILTER_NEAREST);

// Draw the scene into it...
ecs_set(world, scene_view, GFX_Viewport,
        {.camera = scene_cam, .texture = &screen, .rect = {0, 0, 1, 1}});
ecs_set(world, ecs_new_w_pair(world, EcsChildOf, scene_view),
        GFX_SpriteBatch, {.tag = Scene});

// ...and show it in the window as a sprite
GFX_Sprite s = gfx_sprite_create(&screen.texture, 0, 0, 160, 120);
```

- Create it after the first window: it uses the window format.
- Its color is cleared by its first view of the frame, so several viewports can
  share one texture.
- A texture can't be drawn into and sampled in the same pass. Tag the batches
  (`GFX_SpriteBatch.tag`) so the sprite showing the texture isn't drawn into
  it. `examples/engine_render_target.c` is the full example.

## Sprites

`GFX_Sprite` (an atlas rect and a size in pixels) + `GFX_Transform` +
`GFX_SpriteLayer`. A `GFX_SpriteBatch` in a viewport draws them
(`src/sprite/gfx_sprite.c`):

1. collect the sprites (only those with `tag` when set);
2. sort by layer, then texture;
3. upload one instance buffer, owned by the batch;
4. one instanced draw per run of sprites sharing a texture.

The shared shader, pipeline and per-texture materials live in the
`GFX_SpriteRenderer` singleton. Sprites aren't depth tested: layers decide what
is on top.

## Low-level API

The `graphics_*` examples use it directly, without the ECS.

```c
gfx_graphics_init();
GFX_Window *win = gfx_window_init(800, 600, "Example");
gfx_graphics_enable(GFX_GRAPHICS_FEATURE_DEPTH_TESTING); // before pipelines

GFX_Material material = gfx_material_create(
    gfx_shader_unlit(), &(GFX_UnlitUniform){.albedo = {1, 1, 1, 1}}, NULL);
GFX_Pipeline pipeline =
    gfx_pipeline_create(material.shader, gfx_geometry3d_layout());

while (!gfx_window_should_close(win)) {
  gfx_frame_begin();
  gfx_frame_window_begin(win);                 // acquire + clear
  gfx_frame_view_begin(win, (vec4){0, 0, 1, 1}, view_projection);
  gfx_pipeline_bind(&pipeline);
  gfx_material_bind(&material);
  gfx_draw_set_model(model);
  gfx_geometry3d_draw(&geometry);
  gfx_frame_end();                             // submit + present
  gfx_window_events_poll();
}
```

Draws only record inside a view; outside one, or when the window was skipped
(minimized), they do nothing.

### Uniforms

Every shader can read two engine uniforms:

```wgsl
@group(0) @binding(0) var<uniform> view_projection: mat4x4f; // per view
@group(1) @binding(0) var<uniform> model: mat4x4f;           // per draw
```

Both go into one per-frame arena of 4096 matrices (256-byte slots, dynamic
offsets), uploaded once in `gfx_frame_end` (`src/graphics/gfx_uniforms.c`).

### Shaders and materials

A shader is WGSL with `vert_main` and `frag_main`. It declares its material
bindings when created; a material is a shader plus values for them.

```c
// @group(2): binding 0 the uniform, texture i at 1+2i, its sampler at 2+2i
GFX_Shader shader = gfx_shader_create("./assets/lit.wgsl", sizeof(MyUniform), 1);
GFX_Material material = gfx_material_create(&shader, &uniform, textures);
GFX_Pipeline pipeline = gfx_pipeline_create(&shader, gfx_geometry3d_layout());
```

- `0, 0` means no material (no `@group(2)`).
- Missing textures (NULL or failed loads) are white.
- A material keeps a pointer to its shader: the shader must outlive it and not
  move.
- `gfx_shader_unlit()` is the built-in unlit shader for `GFX_Vertex3D` meshes,
  with `GFX_UnlitUniform`.
- A pipeline = shader + vertex layout + the graphics features enabled when it
  was created + the window format, so create it after the first window.

## Rules

- **Lifetimes.** Sprites, materials and viewports hold raw pointers (textures,
  shaders, render textures). What they point at must outlive them and not
  move, so don't point into component storage.
- **Per-view data.** Batches run once per viewport. A buffer written each frame
  must belong to the batch, or every view sees the last write.
- **Systems.** Don't keep pointers to component storage across phases: flecs
  can move it. Systems that touch WebGPU or GLFW run on the main thread.
- **Browser.** No blocking loops (use `gfx_run`), no extra blocking waits, one
  window. Apps package their assets with `--preload-file`.
- **Shaders the engine owns** live next to their code and are compiled in with
  `#embed` (`src/sprite/sprite.wgsl`, `src/graphics/unlit.wgsl`).
