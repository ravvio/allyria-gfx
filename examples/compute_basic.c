#include <gfx/gfx.h>
#include <stdio.h>
#include <stdlib.h>

// Values in the buffer, 32 MiB of f32: more workgroups than one dimension
// allows, so they are split in 2D
#define COUNT (1u << 23)
// Matches @workgroup_size in the shader
#define WORKGROUP_SIZE 64

int main(void) {
  // No window: compute only needs the device
  if (gfx_graphics_init() != GFX_ERR_OK) {
    gfx_graphics_terminate();
    return 1;
  }

  float *values = malloc(COUNT * sizeof(float));
  for (uint32_t i = 0; i < COUNT; i++) {
    values[i] = (float)i;
  }
  GFX_StorageBuffer buffer =
      gfx_storage_buffer_create(COUNT * sizeof(float), false);
  gfx_storage_buffer_write(&buffer, 0, values, COUNT * sizeof(float));

  // One uniform (the factor), one storage buffer (the values)
  GFX_ComputeShader scale = gfx_compute_shader_create(
      "./assets/shaders/compute_scale.wgsl", sizeof(float), 1);

  // Two dispatches in one session, each with its own uniform
  uint32_t groups_x, groups_y;
  gfx_compute_groups(COUNT, WORKGROUP_SIZE, &groups_x, &groups_y);
  const GFX_StorageBuffer *buffers[] = {&buffer};
  float two = 2.0f, three = 3.0f;
  gfx_compute_begin();
  gfx_compute_dispatch(&scale, buffers, &two, groups_x, groups_y, 1);
  gfx_compute_dispatch(&scale, buffers, &three, groups_x, groups_y, 1);
  gfx_compute_end();

  // Every value should now be 6 times its index
  GFX_Error err =
      gfx_storage_buffer_read(&buffer, 0, values, COUNT * sizeof(float));
  uint32_t wrong = 0;
  for (uint32_t i = 0; err == GFX_ERR_OK && i < COUNT; i++) {
    wrong += values[i] != 6.0f * (float)i;
  }
  printf("compute_basic: %u values, %u x %u workgroups, %u wrong\n", COUNT,
         groups_x, groups_y, wrong);

  free(values);
  gfx_compute_shader_destroy(&scale);
  gfx_storage_buffer_destroy(&buffer);
  gfx_graphics_terminate();
  return err != GFX_ERR_OK || wrong > 0;
}
