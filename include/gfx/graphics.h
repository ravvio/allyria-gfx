#ifndef GFX_GRAPHICS_DECL
#define GFX_GRAPHICS_DECL

#include <gfx/core.h>

// Graphics device

GFX_Error gfx_graphics_init(void);
void gfx_graphics_terminate(void);

// Features, enable them before creating pipelines

enum GFX_GraphicsFeature {
  GFX_GRAPHICS_FEATURE_DEPTH_TESTING,
  GFX_GRAPHICS_FEATURE_CULL_BACK,
  GFX_GRAPHICS_FEATURE_ALPHA_BLEND,
};
void gfx_graphics_enable(enum GFX_GraphicsFeature);

#endif
