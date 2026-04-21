#include "../include/gfx/graphics/gfx_graphics.h"

int main(void) {
    if (gfx_graphics_init() != GFX_ERR_OK) {
        gfx_graphics_terminate();
        return 1;
    }

    GFX_Window *win = gfx_window_init(800, 600, "Basic Example");

    GFX_Color_RGBA color = {1.0, 0.3, 0.4, 1.0};
    gfx_color_buffer_set_clear(color);

    while (!gfx_window_should_close(win)) {
        gfx_buffers_clear();

        gfx_window_swap_buffers(win);
        gfx_window_events_poll();
    }

    gfx_graphics_terminate();
    return 0;
}
