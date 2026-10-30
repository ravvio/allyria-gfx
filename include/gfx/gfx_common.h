#ifndef GFX_COMMON_DECL
#define GFX_COMMON_DECL

// Include libraries
#include <math.h>
#include <sys/types.h>

// WebGPU clip space depth is [0, 1]
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include "../cglm/cglm.h"
#include "../flecs/flecs.h"

#include "../stb_ds.h"
#include "../stb_image.h"

// Definitions

typedef u_int8_t GFX_Boolean;

#define GFX_TRUE 1
#define GFX_FALSE 0

// Error

typedef u_int8_t GFX_Error;
#define GFX_ERR_OK 0
#define GFX_ERR_FAILED 1 // Generic error
#define GFX_ERR_GLFW 2   // External libraries failure
#define GFX_ERR_WGPU 3   // WebGPU instance / adapter / device failure

#endif
