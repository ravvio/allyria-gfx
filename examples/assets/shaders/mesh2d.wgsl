@group(0) @binding(0) var<uniform> view_projection: mat4x4f;
@group(1) @binding(0) var<uniform> model: mat4x4f;

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@vertex
fn vert_main(@location(0) position: vec2f, @location(1) uv: vec2f) -> VertexOutput {
    var output: VertexOutput;
    output.position = view_projection * model * vec4f(position, 0.0, 1.0);
    output.uv = uv;
    return output;
}

@fragment
fn frag_main(input: VertexOutput) -> @location(0) vec4f {
    return vec4f(input.uv, 0.0, 1.0);
}
