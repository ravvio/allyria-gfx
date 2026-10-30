@group(0) @binding(0) var<uniform> view_projection: mat4x4f;
@group(1) @binding(0) var<uniform> model: mat4x4f;

// Material layout (sizeof(vec4), 1 texture)
@group(2) @binding(0) var<uniform> albedo: vec4f;
@group(2) @binding(1) var albedo_tex: texture_2d<f32>;
@group(2) @binding(2) var albedo_sampler: sampler;

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@vertex
fn vs_main(@location(0) position: vec3f, @location(2) uv: vec2f) -> VertexOutput {
    var output: VertexOutput;
    output.position = view_projection * model * vec4f(position, 1.0);
    output.uv = uv;
    return output;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4f {
    return albedo * textureSample(albedo_tex, albedo_sampler, input.uv);
}
