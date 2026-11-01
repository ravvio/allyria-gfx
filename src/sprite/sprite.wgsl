@group(0) @binding(0) var<uniform> view_projection: mat4x4f;

// Material layout (no uniform, 1 texture)
@group(2) @binding(1) var sprite_tex: texture_2d<f32>;
@group(2) @binding(2) var sprite_sampler: sampler;

// Per instance (GFX_SpriteInstance)
struct Instance {
    @location(0) model_0: vec4f,
    @location(1) model_1: vec4f,
    @location(2) model_2: vec4f,
    @location(3) model_3: vec4f,
    // Atlas rect: offset xy, scale zw (top left origin)
    @location(4) uv_rect: vec4f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@vertex
fn vs_main(@builtin(vertex_index) index: u32, instance: Instance) -> VertexOutput {
    // Unit quad centered on the origin, two counter clockwise triangles
    var corners = array<vec2f, 6>(
        vec2f(-0.5, -0.5), vec2f(0.5, -0.5), vec2f(0.5, 0.5),
        vec2f(0.5, 0.5), vec2f(-0.5, 0.5), vec2f(-0.5, -0.5),
    );
    let corner = corners[index];
    let model = mat4x4f(instance.model_0, instance.model_1, instance.model_2, instance.model_3);

    var output: VertexOutput;
    output.position = view_projection * model * vec4f(corner, 0.0, 1.0);
    // The quad y goes up, the texture v goes down
    output.uv = instance.uv_rect.xy + vec2f(corner.x + 0.5, 0.5 - corner.y) * instance.uv_rect.zw;
    return output;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4f {
    return textureSample(sprite_tex, sprite_sampler, input.uv);
}
