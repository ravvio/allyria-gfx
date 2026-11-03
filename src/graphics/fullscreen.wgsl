
struct FullscreenOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
};

@vertex
fn vert_main(@builtin(vertex_index) i: u32) -> FullscreenOutput {
    let pos = vec2f(f32((i << 1u) & 2u), f32(i & 2u)); // (0, 0), (2, 0), (0, 2)
    var out: FullscreenOutput;
    out.position = vec4f(pos * 2.0 - 1.0, 0.0, 1.0); // counter-clockwise
    out.uv = vec2f(pos.x, 1.0 - pos.y); // texture v points down
    return out;
}
