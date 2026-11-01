#ifndef GFX_WINDOW_DECL
#define GFX_WINDOW_DECL

#include <gfx/core.h>

// Window

typedef void GFX_Window;

GFX_Window *gfx_window_init(int w, int h, const char *window_name);
void gfx_window_destroy(GFX_Window *window);

void gfx_window_size(GFX_Window *window, int *w, int *h);
void gfx_window_size_set(GFX_Window *window, int w, int h);
void gfx_window_scale(GFX_Window *window, float *sx, float *sy);
int gfx_window_should_close(GFX_Window *window);
void gfx_window_events_poll(void);

// Frame

void gfx_color_buffer_set_clear(const GFX_ColorRGBA color);
void gfx_frame_begin(GFX_Window *window);
void gfx_frame_end(GFX_Window *window);

#endif
