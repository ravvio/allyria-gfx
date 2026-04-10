// This is the header that includes internal functions

#include "gfx.h"

#ifdef GFX_NDEBUG
#define GLCall(x) x;
#else
#define GLCall(x)                                                              \
  gfx_gl_errors_clear();                                                       \
  x;                                                                           \
  gfx_gl_errors_log(#x, __FILE__, __LINE__);
#endif

#ifdef GFX_DEBUG
#define GLFWCall(x) x;
#else
#define GLFWCall(x)                                                            \
  gfx_glfw_errors_clear();                                                     \
  x;                                                                           \
  gfx_glfw_errors_log(#x, __FILE__, __LINE__);
#endif

// Error checking glfw

void gfx_glfw_errors_clear(void);
int gfx_glfw_errors_check(void);
int gfx_glfw_errors_log(const char *function, const char *file, int line);

// Error checking gl

void gfx_gl_errors_clear();
int gfx_gl_errors_check();
int gfx_gl_errors_log(const char *function, const char *file, int line);

// Types

u_int32_t gfx_gl_type_get_size(u_int32_t type);

// Conversion

GLenum gfx_gl_wrap(enum GFX_Wrap wrap);
GLenum gfx_gl_filter(enum GFX_TextureFilter filter);
GLenum gfx_gl_min_filter(enum GFX_TextureFilter filter);
