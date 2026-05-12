#include "../../include/gfx/graphics/gfx_gl.h"

void gfx_glfw_errors_clear() {
  while (glfwGetError(NULL) != GLFW_NO_ERROR) {
  }
}

int gfx_glfw_errors_check() {
  int ok = GFX_TRUE;
  for (int err = glfwGetError(NULL); err != GLFW_NO_ERROR;
       err = glfwGetError(NULL)) {
    fprintf(stderr, "[e] GLFW Error %d", err);
    ok = -1;
  }
  return ok;
}

int gfx_glfw_errors_log(const char *function, const char *file, int line) {
  int ok = GFX_TRUE;
  const char *desc;
  for (int err = glfwGetError(&desc); err != GLFW_NO_ERROR;
       err = glfwGetError(NULL)) {
    fprintf(stderr, "[e] GLFW Error %d: %s\n| %s\n| at %s:%d\n", err,
            desc, function, file, line);
    ok = -1;
  }
  return ok;
}
