// Internal to the library: shared WebGPU state and backend functions

#ifndef GFX_INTERNAL_DECL
#define GFX_INTERNAL_DECL

#include <gfx/core.h>

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
  // Pipelines without a material: group 0 frame, group 1 object
  WGPUPipelineLayout pipeline_layout;
  WGPUColor clear_color;
  // Enabled GFX_GraphicsFeature bits, baked into pipelines
  uint32_t features;

  // Current frame, set between gfx_frame_begin and gfx_frame_end
  WGPUTexture texture;
  WGPUTextureView view;
  WGPUCommandEncoder encoder;
  // NULL when the frame was skipped, draw calls are then ignored
  WGPURenderPassEncoder pass;
} _GFX_State;
extern _GFX_State _gfx;

// Depth buffer of every window, pipelines must match it
#define GFX_DEPTH_FORMAT WGPUTextureFormat_Depth24Plus

// Uniforms

GFX_Error gfx_uniforms_init(void);
void gfx_uniforms_terminate(void);
void gfx_uniforms_frame_begin(void);
void gfx_uniforms_frame_end(void);
// Frame and object groups, plus the material group if not NULL
WGPUPipelineLayout gfx_uniforms_pipeline_layout(WGPUBindGroupLayout material);

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
