#ifndef GFX_COMMON_DECL
#define GFX_COMMON_DECL

// Include libraries
#include <math.h>
#include <sys/types.h>

#define GLAD_GL_IMPLEMENTATION
#include "../glad/gl.h"
#include <GLFW/glfw3.h>

#include "../cglm/cglm.h"
#include "../flecs/flecs.h"

#include "../stb_image.h"

// Definitions

typedef u_int8_t GFX_Boolean;

#define GFX_TRUE 1
#define GFX_FALSE 2

// Error

typedef u_int8_t GFX_Error;
#define GFX_ERR_OK 0
#define GFX_ERR_FAILED 1 // Generic error
#define GFX_ERR_GLFW 2   // External libraries failure

#endif
