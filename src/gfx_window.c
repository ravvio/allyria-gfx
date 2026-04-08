#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include "../include/gfx/gfx.h"

typedef GLFWwindow _GFX_Window;

GFX_Window *gfx_window_create(int w, int h, char* window_name) {
  _GFX_Window *window;

  // Initialize GLFW
  if (!glfwInit()) {
    return NULL;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a window
  window = glfwCreateWindow(800, 600, window_name, NULL, NULL);
  if (!window) {
    glfwTerminate();
    return NULL;
  }
  // Make window's the current context
  glfwMakeContextCurrent(window);

  // Initialie GLEW
  GLenum err = glewInit();

  if (err != GLEW_OK && err != 4) {
    fprintf(stderr, "[e] error: %s, code: %d\n", glewGetErrorString(err), err);
    glfwTerminate();
    return NULL;
  }
  fprintf(stdout, "[i] status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

  // Set clear color
  glClearColor(0.1, 0.25, 0.35, 1.0);

  return window;
}

// Get the size of the window
void gfx_window_size(GFX_Window *window, int *w, int *h) {
  glfwGetWindowSize((_GFX_Window*)window, w, h);
}

// Set the size of the window
void gfx_window_size_set(GFX_Window *window, int w, int h) {
  glfwSetWindowSize((_GFX_Window*)window, w, h);
}

// Should the window be closed
int gfx_window_should_close(GFX_Window *window) {
  return glfwWindowShouldClose((_GFX_Window*)window);
}

// Swap front and back buffers
void gfx_window_swap(GFX_Window *window) {
  glfwSwapBuffers((_GFX_Window*)window);
}

// Poll for and process events
void gfx_events_poll() {
  glfwPollEvents();
}

// Terminate the process
void gfx_terminate() {
  glfwTerminate();
}
