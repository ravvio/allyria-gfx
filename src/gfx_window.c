#include "../include/gfx/gfx_gl.h"

typedef GLFWwindow _GFX_Window;

// Initialize the gfx library
GFX_Error gfx_init() {
  // Initialize GLFW
  if (glfwInit() == GLFW_FALSE) {
    gfx_glfw_errors_check();
    return GFX_ERR_GLFW;
  }
  fprintf(stdout, "[i] status: Using GLFW %s\n", glfwGetVersionString());

  return GFX_ERR_OK;
}

// Terminate the gfx library
void gfx_terminate() { glfwTerminate(); }

GFX_Window *gfx_window_init(int w, int h, char *window_name) {
  // Hint
  GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3));
  GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3));
  GLFWCall(glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE));

  // Create a window
  GLFWCall(GFX_Window *window = glfwCreateWindow(800, 600, window_name, NULL, NULL));
  if (!window) {
    return NULL;
  }
  // Make window's the current context
  GLFWCall(glfwMakeContextCurrent(window));

  // Load GL on the current context
  int version = gladLoadGL(glfwGetProcAddress);
  fprintf(stderr, "[i] status: Using GL %d.%d\n", GLAD_VERSION_MAJOR(version),
          GLAD_VERSION_MINOR(version));

  // Enable depth testing
  GLCall(glEnable(GL_DEPTH_TEST));

  return window;
}

// Get the size of the window
void gfx_window_size(GFX_Window *window, int *w, int *h) {
  GLFWCall(glfwGetWindowSize((_GFX_Window *)window, w, h));
}

// Set the size of the window
void gfx_window_size_set(GFX_Window *window, int w, int h) {
  GLFWCall(glfwSetWindowSize((_GFX_Window *)window, w, h));
}

// Should the window be closed
int gfx_window_should_close(GFX_Window *window) {
  GLFWCall(int result = glfwWindowShouldClose((_GFX_Window *)window));
  return result;
}

// Swap front and back buffers
void gfx_window_swap_buffers(GFX_Window *window) {
  GLFWCall(glfwSwapBuffers((_GFX_Window *)window));
}

// Clear the color buffer
void gfx_color_buffer_clear() {
  GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

// Clear the depth buffer
void gfx_depth_buffer_clear() {
  GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

// Clear all buffers
void gfx_buffers_clear() {
  GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

// Set the color buffer clear
void gfx_color_buffer_set_clear(GFX_Color_RGBA color) {
  GLCall(glClearColor(color[0], color[1], color[2], color[3]));
}

// Set the depth buffer clear color
void gfx_depth_buffer_set_clear(float_t depth) {
  GLCall(glClearDepth(depth));
}
