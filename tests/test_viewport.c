// Checks viewport pixels and camera aspect fitting, no GPU needed

#undef NDEBUG
#include "gfx_internal.h"
#include <assert.h>
#include <gfx/camera.h>
#include <math.h>
#include <stdio.h>

static void check_pixels(float x, float y, float w, float h, uint32_t width,
                         uint32_t height, uint32_t px, uint32_t py, uint32_t pw,
                         uint32_t ph) {
  uint32_t out[4];
  gfx_viewport_pixels((float[4]){x, y, w, h}, width, height, out);
  assert(out[0] == px && out[1] == py && out[2] == pw && out[3] == ph);
}

static void check_fit(enum GFX_CameraScalingMode mode, float aspect) {
  float s = 2.0f;
  GFX_Camera camera = {
      .scaling_mode = mode,
      .view = GLM_MAT4_IDENTITY_INIT,
      .model = GLM_MAT4_IDENTITY_INIT,
  };
  glm_ortho(-s, s, -s, s, -100.0f, 100.0f, camera.projection);

  mat4 expected;
  if (mode == GFX_CAMERA_SCALING_MODE_FIT_HEIGHT) {
    glm_ortho(-s * aspect, s * aspect, -s, s, -100.0f, 100.0f, expected);
  } else {
    glm_ortho(-s, s, -s / aspect, s / aspect, -100.0f, 100.0f, expected);
  }

  mat4 result;
  gfx_camera_view_projection(camera, aspect, result);
  for (int c = 0; c < 4; c++) {
    for (int r = 0; r < 4; r++) {
      assert(fabsf(result[c][r] - expected[c][r]) < 1e-6f);
    }
  }
}

int main(void) {
  // Whole image
  check_pixels(0, 0, 1, 1, 800, 600, 0, 0, 800, 600);

  // Halves of an odd width share the middle edge, no gap
  check_pixels(0, 0, 0.5f, 1, 801, 600, 0, 0, 401, 600);
  check_pixels(0.5f, 0, 0.5f, 1, 801, 600, 401, 0, 400, 600);

  // Clamped inside the image
  check_pixels(-0.5f, 0.75f, 1, 0.5f, 800, 600, 0, 450, 400, 150);

  // Empty
  check_pixels(0.2f, 0.2f, 0, 0.5f, 800, 600, 160, 120, 0, 300);

  // The stretched square projection matches an ortho of that aspect
  check_fit(GFX_CAMERA_SCALING_MODE_FIT_HEIGHT, 4.0f / 3.0f);
  check_fit(GFX_CAMERA_SCALING_MODE_FIT_WIDTH, 4.0f / 3.0f);
  check_fit(GFX_CAMERA_SCALING_MODE_FIT_HEIGHT, 0.5f);

  printf("viewport ok\n");
  return 0;
}
