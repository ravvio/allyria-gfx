#ifndef GFX_CORE_DECL
#define GFX_CORE_DECL

#include <stdint.h>

// WebGPU clip space depth is [0, 1]
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>

// Color

typedef vec3 GFX_ColorRGB;
typedef vec4 GFX_ColorRGBA;

// Error

typedef enum : uint8_t {
  GFX_ERR_OK,
  GFX_ERR_FAILED, // Generic error
  GFX_ERR_GLFW,   // External libraries failure
  GFX_ERR_WGPU,   // WebGPU instance / adapter / device failure
} GFX_Error;

#endif
