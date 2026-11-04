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

// Limits

typedef struct {
  uint64_t max_buffer_size;
  uint64_t max_storage_binding_size;
  uint32_t max_workgroups_per_dimension;
  uint32_t max_workgroup_invocations;
} GFX_GraphicsLimits;

// Limits of the device, after gfx_graphics_init
GFX_GraphicsLimits gfx_graphics_limits(void);
// The most the adapter could give, also before gfx_graphics_init
GFX_GraphicsLimits gfx_graphics_adapter_limits(void);
// Ask for more than the defaults, before gfx_graphics_init. Fields left at 0
// keep the default, init fails if the adapter can't provide one.
void gfx_graphics_require_limits(const GFX_GraphicsLimits *limits);

#endif
