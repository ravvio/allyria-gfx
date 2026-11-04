// Multiplies every value by factor
@group(0) @binding(0) var<uniform> factor: f32;
@group(0) @binding(1) var<storage, read_write> values: array<f32>;

@compute @workgroup_size(64)
fn comp_main(@builtin(workgroup_id) group: vec3u,
             @builtin(num_workgroups) groups: vec3u,
             @builtin(local_invocation_index) local: u32) {
    // Workgroups can be split in 2D past the dimension limit
    let i = (group.y * groups.x + group.x) * 64u + local;
    if (i >= arrayLength(&values)) {
        return;
    }
    values[i] = values[i] * factor;
}
