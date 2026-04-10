#include "../include/gfx/gfx_gl.h"

// Poll for and process events
void gfx_events_poll() { GLFWCall(glfwPollEvents()); }
