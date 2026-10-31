# AGENTS.md

Small C99 game engine: **flecs** (ECS) + **WebGPU** (Dawn natively, emdawnwebgpu in the browser) + **GLFW** + **cglm** + **stb_image / stb_ds**.

## Build & run

```sh
cmake -B build                 # Debug by default; fetches glfw, cglm, Dawn, glfw3webgpu
cmake --build build --parallel
ctest --test-dir build
cd build/Debug/<arch> && ./engine_sprites   # <arch> = arm64 / x86_64
```

- Browser build: `emcmake cmake -B build/web` then `cmake --build build/web --parallel`. Output is `build/web/Debug/x86/*.html`; serve it over HTTP (see `README.md`). Emscripten needs Python 3.10+: set `EMSDK_PYTHON` if `python3` is older.
- Examples must be run from `build/Debug/<arch>/`: asset paths are relative (`./assets/...`) and `assets/` is copied there at **configure** time. Re-run `cmake -B build` after adding/changing assets or shaders.
- GPU-free logic has CTest checks in `tests/`. Add one when you write non-trivial logic that runs without a device.
- Everything else is verified by building and running the affected examples (`examples/*.c`, one executable each, registered with `add_exe()` in `CMakeLists.txt`). WebGPU errors print as `[e] WebGPU error`.
- New `.c` files in `src/` must be added to `GFXSOURCES` in `CMakeLists.txt`.

## Layout

| Path | What |
|---|---|
| `include/gfx/gfx.h` | Engine public API: settings, `gfx_run`, ECS components (Transform, Camera, Surface, Uniforms, Engine), render phases |
| `include/gfx/gfx_common.h` | Includes every third-party header, `GFX_Error`, `GFX_Boolean` |
| `include/gfx/graphics/gfx_graphics.h` | Low-level graphics API: window, frame, buffers, shaders, pipelines, uniforms, textures, materials, geometry |
| `include/gfx/graphics/gfx_wgpu.h` | Internal backend header: GLFW, shared WebGPU state (`_gfx`), `GLFWCall`, internal init functions |
| `src/graphics/gfx_window.c` | WebGPU instance/adapter/device, window + surface + depth buffer, frame begin/end |
| `src/graphics/gfx_buffers.c` | Vertex/index buffers, vertex layout, draw calls |
| `src/graphics/gfx_shader.c` | WGSL shader modules, render pipelines |
| `src/graphics/gfx_uniforms.c` | Shared bind group/pipeline layouts, view-projection buffer, per-draw model arena |
| `src/graphics/gfx_texture.c` | RGBA8 textures (from pixels or `stb_image`) with their sampler, CPU mipmaps |
| `src/graphics/gfx_material.c` | Material layouts (defined by the caller), materials, white fallback texture |
| `src/graphics/gfx_geometry2d.c`, `gfx_geometry3d.c` | Meshes (`GFX_Vertex2D` / `GFX_Vertex3D`), quad and cuboid shapes, their vertex layouts |
| `src/renderer/` | Render phases & frame systems, surface (window + DPI) |
| `src/sprite/gfx_sprite.c` | Instanced sprites: query, sort by (layer, texture), one instance upload, one draw per texture run |
| `src/camera/`, `src/sprite/`, `src/gfx_*.c` | flecs modules |
| `assets/shaders/` | WGSL, one file per pipeline with `vs_main` / `fs_main` |
| `tests/` | CTest checks for GPU-free logic |
| `include/{cglm,flecs}`, `include/stb_*.h` | Vendored third-party code — don't edit |

## Conventions

- Types `GFX_PascalCase`, functions `gfx_snake_case`, enums `GFX_UPPER_CASE`. Integer types from `<stdint.h>` (`uint32_t`), not BSD `u_int32_t` (missing on Emscripten's libc).
- flecs modules: `void Gfx<Name>Import(ecs_world_t *)` using `ECS_MODULE` + `ecs_set_name_prefix(world, "Gfx")`; imported with `ECS_IMPORT(world, Gfx<Name>)`.
- Components: `extern ECS_COMPONENT_DECLARE(T)` in the header, `ECS_COMPONENT_DECLARE(T)` in the `.c`, `ECS_COMPONENT_DEFINE` in the import function. Components owning GPU/heap resources get `ECS_DTOR` + `ECS_MOVE` hooks (see `gfx_surface.c`, `gfx_sprite.c`).
- System callbacks are named `_gfx_sys_<what>`. Systems must not keep pointers to component storage across phases (flecs can move it): copy what they need, as the sprite renderer does.
- Includes use relative paths (`#include "../../include/gfx/gfx.h"`), not include dirs.
- The backend boundary is `gfx_graphics.h` + `src/graphics/`. Modules above it (`renderer/`, `sprite/`) only use that API, never `wgpu*` calls directly.
- Resource structs are returned by value from `*_create` / `*_load` and released with `*_destroy(const T *)`.
- `*_bind` / `gfx_draw_*` record into the current frame's render pass. Call them between `gfx_frame_begin` and `gfx_frame_end`; they do nothing when the frame was skipped.
- Pipelines (`gfx_pipeline_create`) read the enabled graphics features and the surface format when created: enable features and create the window first.
- Uniforms follow a fixed contract. Pipeline layouts are always explicit (never `'auto'`), and groups 0 and 1 are shared by every pipeline:
  - `@group(0) @binding(0) var<uniform> view_projection: mat4x4f;` set once per frame with `gfx_frame_set_view_projection`;
  - `@group(1) @binding(0) var<uniform> model: mat4x4f;` set per draw with `gfx_draw_set_model`. It goes into a 4096-slot per-frame arena (dynamic offsets) uploaded once in `gfx_frame_end`;
  - `@group(2)` is the material. Its layout is defined by the caller with `gfx_material_layout_create(uniform_size, texture_count)` and passed to `gfx_pipeline_create` (NULL = no group 2):
    - binding 0 is the uniform (only if the size is > 0);
    - texture *i* is at binding `1+2i`, its sampler at `2+2i`;
    - NULL or failed textures fall back to white;
    - the engine doesn't name material fields; the shader and the caller's uniform struct do.
  - Never `writeBuffer` per-draw data into one shared buffer region: every draw would see the last value.
- Instanced data (sprites) uses a vertex buffer with `GFX_VertexLayout.step = WGPUVertexStepMode_Instance` and `gfx_draw_instanced`.
- Coordinates: clip depth is `[0, 1]` (`CGLM_FORCE_DEPTH_ZERO_TO_ONE` is defined in `gfx_common.h`, so cglm projections match), front faces are counter-clockwise, texture origin is top-left.
- Logging: `fprintf(stderr, "[i] ...")` for info, `"[e] ..."` for errors. Per-frame logs only under `#ifdef GFX_VERBOSE`.
- Errors: return `GFX_Error` (`GFX_ERR_OK` == 0). Wrap GLFW calls in `GLFWCall(...)` (compiled out with `GFX_NDEBUG`). WebGPU errors come from the device's uncaptured-error callback.
- Dynamic arrays and hashmaps use `stb_ds`. Math uses cglm (`mat4`, `vec3`, `glm_*`).
- Design decisions based on outside sources (specs, articles, other engines): cite them and add them to `SOURCES.md` (references and a short note on consensus).
- Formatting: `.clang-format` (LLVM style, 2-space indent, 80 cols). Format the files you touch. Vendored code is listed in `.clang-format-ignore` (read by clang-format 18+).

## Frame / ECS flow

`gfx_run(world)` calls `ecs_progress()` every frame (a loop natively, `requestAnimationFrame` in the browser), which runs:
1. `EcsPreUpdate`: quit if the window should close.
2. `EcsOnUpdate` / `EcsPostUpdate`: gameplay, sync `GFX_Transform` → camera model.
3. Render phases (defined in `gfx_renderer.c`, chained after `EcsOnStore`):
   `GfxPreRender` (surface sync, camera matrix into `GFX_Uniforms`, `gfx_frame_set_view_projection`) → `GfxStartRender` (`gfx_frame_begin`: acquire surface texture, begin cleared render pass) → `GfxRenderObjects` (draw, e.g. sprites) → `GfxRenderGizmos` → `GfxRenderGUI` → `GfxEndRender` (`gfx_frame_end`: upload the model arena, submit, present) → `GfxPostRender` (poll events).

`GFX_Engine` is a singleton holding the surface, uniforms and main camera entity ids.

Every frame's render pass has a `Depth24Plus` depth attachment, owned by the window and recreated on resize. Every pipeline therefore declares a depth state; without `GFX_GRAPHICS_FEATURE_DEPTH_TESTING` it uses compare `Always` with writes off.

WebGPU objects (instance, adapter, device, queue, current frame's encoder/pass) live in the `_gfx` struct (`gfx_wgpu.h`, defined in `gfx_window.c`). A frame can be skipped (window minimized, surface outdated): `gfx_frame_end` then does nothing, and draw code must handle a NULL render pass.

## Targets

Native is the default. Configuring with `emcmake cmake` builds for the browser (`EMSCRIPTEN` is set).

- Both use the same Dawn release (`DAWN_TAG` / `DAWN_REV` in `CMakeLists.txt`):
  - **Native:** Dawn's prebuilt archive, pinned with a SHA256 per platform (target `dawn::webgpu_dawn`). To upgrade, change the tag/revision and all hashes.
  - **Browser:** Dawn's `emdawnwebgpu` remote port from the same release, also pinned by hash.
- Only use the standard `webgpu.h` API. Code for one target goes behind `#ifdef __EMSCRIPTEN__` / `#ifndef __EMSCRIPTEN__`, kept inside `src/graphics/` or `gfx_run`.
- Engine code must work in the browser:
  - **No blocking main loop.** Use `gfx_run`. The `graphics_*` examples have their own loop, so only `engine_*` examples are built for the browser.
  - **Adapter/device requests are async.** Init waits on them with `wgpuInstanceWaitAny` (instance feature `TimedWaitAny`), which needs `-sASYNCIFY` in the browser. Don't add other blocking waits.
  - **No `wgpuSurfacePresent` / `wgpuInstanceProcessEvents`** in the browser (it presents on its own).
- Browser link setup (`CMakeLists.txt`):
  - GLFW comes from Emscripten's `contrib.glfw3` port; the built-in `-sUSE_GLFW=3` is GLFW 3.2 and lacks `glfwGetError`.
  - Executables link with `em++` (emdawnwebgpu is C++).
  - flecs needs `-sSTACK_SIZE=1mb` (it overflows the default 64 KB in `ecs_init`).
  - Assets are packaged with `--preload-file` at `/assets`, so `./assets/...` paths work unchanged.
- **Threads:** the engine must work with any flecs thread count but must not require threads. Games can call `ecs_set_threads` (native only).
  - Systems that touch WebGPU or GLFW run on the main thread: never mark them `multi_threaded`.
  - Browser builds are single-threaded; threads there would need `-pthread` plus COOP/COEP headers.

## Gotchas

- flecs is built from the vendored `include/flecs/flecs.c`; header and source must stay the same version (a mismatch crashes at startup). Upgrade both files together.
- cglm is used from the vendored `include/cglm` headers; the FetchContent cglm `master` is only linked, so it can drift.
- `STB_IMAGE_IMPLEMENTATION` / `STB_DS_IMPLEMENTATION` are defined only in `src/gfx.c`.
- Warnings (`-Wall -Wextra -pedantic`) are only enabled on example targets, not on the `gfx` library.
- Dawn is a C++ static library, so the project enables `CXX` for linking. Engine code stays C99.
- Only Dawn's Release archives are downloaded: Windows Debug builds would need the Debug archive (MSVC runtime mismatch).
- WebGPU has no `generateMipmap`.
  - **Linear textures** get a full mip chain built on the CPU at creation (2×2 box filter). Averaging non-premultiplied alpha can darken transparent edges in the smaller mips.
  - **Nearest textures** keep a single level, so atlas cells don't bleed.
  - Move to GPU generation if load time matters or render-to-texture is added.
- The uniform arena holds 4096 draws per frame; past that, extra draws reuse the last model and an error is logged once per frame.
- Editors may show `std::is_empty_v` errors in headers that include `flecs.h`: the editor parses the header as C++. It's not a build error.
