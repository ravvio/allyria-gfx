# GFX Engine

Using:
- opengl (with glad)
- stb_image
- cglm

## Build

Requirements: CMake 3.22+ and a C/C++ compiler. The dependencies (GLFW, Dawn,
cglm, glfw3webgpu) are downloaded by CMake.

### Native

Configure the build. It defaults to Debug and downloads the dependencies on the
first run:

```sh
cmake -B build
```

Build the library, the examples and the tests, compiling files in parallel:

```sh
cmake --build build --parallel
```

Run the tests:

```sh
ctest --test-dir build
```

Examples are written to `build/Debug/<arch>/` (e.g. `arm64` or `x86_64`) and
must be run from there, because assets are loaded from `./assets`:

```sh
cd build/Debug/<arch>
./engine_sprites
```

The assets are copied at configure time, so re-run `cmake -B build` after
changing anything in `assets/`.

### Browser

The browser build needs [Emscripten](https://emscripten.org/docs/getting_started/downloads.html)
4.0.10 or newer. Emscripten itself needs Python 3.10 or newer: if the `python3`
on your `PATH` is older, point `EMSDK_PYTHON` to a newer interpreter.

Configure the build in `build/web` with Emscripten's CMake wrapper:

```sh
emcmake cmake -B build/web
```

Build the examples as `.html` pages:

```sh
cmake --build build/web --parallel
```

Only the `engine_*` examples are built for the browser, the `graphics_*` ones
run their own blocking loop.

## Serve

The pages must be served over HTTP, opening the files directly doesn't work.
Any static file server works, for example Python's (bound to localhost only):

```sh
python3 -m http.server 8000 --bind 127.0.0.1 -d build/web/Debug/x86
```

Then open <http://localhost:8000/engine_sprites.html> in a browser with WebGPU
support (recent Chrome, Edge or Safari).

## Extensions
https://gen.glad.sh/#generator=c&api=gl%3D4.6&profile=gl%3Dcore%2Cgles1%3Dcommon&extensions=GL_APPLE_element_array%2CGL_APPLE_vertex_array_object%2CGL_APPLE_vertex_array_range%2CGL_ARB_vertex_array_bgra%2CGL_ARB_vertex_array_object%2CGL_ATI_element_array%2CGL_ATI_vertex_array_object%2CGL_EXT_compiled_vertex_array%2CGL_EXT_gpu_program_parameters%2CGL_EXT_gpu_shader4%2CGL_EXT_index_array_formats%2CGL_EXT_packed_depth_stencil%2CGL_EXT_packed_float%2CGL_EXT_packed_pixels%2CGL_EXT_pixel_buffer_object%2CGL_EXT_pixel_transform%2CGL_EXT_pixel_transform_color_table%2CGL_EXT_shader_framebuffer_fetch%2CGL_EXT_shader_framebuffer_fetch_non_coherent%2CGL_EXT_shader_image_load_formatted%2CGL_EXT_shader_image_load_store%2CGL_EXT_shader_integer_mix%2CGL_EXT_shader_samples_identical%2CGL_EXT_subtexture%2CGL_EXT_texture%2CGL_EXT_texture3D%2CGL_EXT_texture_array%2CGL_EXT_texture_buffer_object%2CGL_EXT_texture_compression_latc%2CGL_EXT_texture_compression_rgtc%2CGL_EXT_texture_compression_s3tc%2CGL_EXT_texture_cube_map%2CGL_EXT_texture_env_add%2CGL_EXT_texture_env_combine%2CGL_EXT_texture_env_dot3%2CGL_EXT_texture_filter_anisotropic%2CGL_EXT_texture_filter_minmax%2CGL_EXT_texture_integer%2CGL_EXT_texture_lod_bias%2CGL_EXT_texture_mirror_clamp%2CGL_EXT_texture_object%2CGL_EXT_texture_perturb_normal%2CGL_EXT_texture_shadow_lod%2CGL_EXT_texture_shared_exponent%2CGL_EXT_texture_snorm%2CGL_EXT_texture_sRGB%2CGL_EXT_texture_sRGB_decode%2CGL_EXT_texture_sRGB_R8%2CGL_EXT_texture_sRGB_RG8%2CGL_EXT_texture_storage%2CGL_EXT_texture_swizzle%2CGL_EXT_vertex_array%2CGL_EXT_vertex_array_bgra%2CGL_IBM_vertex_array_lists%2CGL_NV_vertex_array_range%2CGL_NV_vertex_array_range2
