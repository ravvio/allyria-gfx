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

typedef uint8_t GFX_Error;
#define GFX_ERR_OK 0
#define GFX_ERR_FAILED 1 // Generic error
#define GFX_ERR_GLFW 2   // External libraries failure
#define GFX_ERR_WGPU 3   // WebGPU instance / adapter / device failure

#endif
