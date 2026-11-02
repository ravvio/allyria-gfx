// Internal to the library: shared WebGPU state and backend functions

#ifndef GFX_INTERNAL_DECL
#define GFX_INTERNAL_DECL

#include <gfx/core.h>
#include <gfx/texture.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>

// WebGPU state, the equivalent of the GL context
typedef struct {
  WGPUInstance instance;
  WGPUAdapter adapter;
  WGPUDevice device;
  WGPUQueue queue;
  WGPUTextureFormat surface_format;
  // Layout of pipelines without a material
  WGPUPipelineLayout pipeline_layout;
  WGPUColor clear_color;
  // Enabled graphics features
  uint32_t features;

  // Current frame
  WGPUTexture texture;
  WGPUTextureView view;
  WGPUTextureView depth_view;
  // Size of the image being drawn this frame, in pixels
  uint32_t width;
  uint32_t height;
  WGPUCommandEncoder encoder;
  // NULL when the frame was skipped
  WGPURenderPassEncoder pass;
  // What the pass draws into, NULL for the window
  const GFX_RenderTexture *pass_target;
  // Views begun this frame
  uint32_t view_count;
  // Frames begun so far
  uint64_t frame;
} _GFX_State;
extern _GFX_State _gfx;

// Depth buffer format of every window
#define GFX_DEPTH_FORMAT WGPUTextureFormat_Depth24Plus
WGPUTexture gfx_depth_texture_create(uint32_t width, uint32_t height);

// Uniforms

GFX_Error gfx_uniforms_init(void);
void gfx_uniforms_terminate(void);
void gfx_uniforms_frame_begin(void);
void gfx_uniforms_frame_end(void);
// Prepare uniforms for the new pass
void gfx_uniforms_view_begin(mat4 view_projection);
// Pipeline layout with an optional material
WGPUPipelineLayout gfx_uniforms_pipeline_layout(WGPUBindGroupLayout material);

// Viewports

// Pixel position and size of a viewport, clamped to the image
void gfx_viewport_pixels(const float rect[4], uint32_t width, uint32_t height,
                         uint32_t out[4]);

// Texture mipmaps

uint32_t gfx_texture_mip_count(int width, int height);
void gfx_texture_downsample(const uint8_t *src, int src_w, int src_h,
                            uint8_t *dst, int dst_w, int dst_h);

// Materials

void gfx_materials_init(void);
void gfx_materials_terminate(void);

#ifdef GFX_NDEBUG
#define GLFWCall(x) x;
#else
#define GLFWCall(x)                                                            \
  gfx_glfw_errors_clear();                                                     \
  x;                                                                           \
  gfx_glfw_errors_log(#x, __FILE__, __LINE__);
#endif

// Error checking glfw

// The check and log return false if there were errors
void gfx_glfw_errors_clear(void);
bool gfx_glfw_errors_check(void);
bool gfx_glfw_errors_log(const char *function, const char *file, int line);

#endif
