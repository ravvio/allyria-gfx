#include "gfx_internal.h"
#include <stdio.h>

void gfx_glfw_errors_clear() {
  while (glfwGetError(NULL) != GLFW_NO_ERROR) {
  }
}

bool gfx_glfw_errors_check() {
  bool ok = true;
  for (int err = glfwGetError(NULL); err != GLFW_NO_ERROR;
       err = glfwGetError(NULL)) {
    fprintf(stderr, "[e] GLFW Error %d", err);
    ok = false;
  }
  return ok;
}

bool gfx_glfw_errors_log(const char *function, const char *file, int line) {
  bool ok = true;
  const char *desc;
  for (int err = glfwGetError(&desc); err != GLFW_NO_ERROR;
       err = glfwGetError(NULL)) {
    fprintf(stderr, "[e] GLFW Error %d: %s\n| %s\n| at %s:%d\n", err, desc,
            function, file, line);
    ok = false;
  }
  return ok;
}
