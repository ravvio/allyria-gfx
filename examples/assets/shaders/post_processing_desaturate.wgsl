@group(2) @binding(0) var<uniform> strength: f32;
@group(2) @binding(1) var tex: texture_2d<f32>;
@group(2) @binding(2) var scene_sampler: sampler;

@fragment
fn frag_main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    let color = textureSample(tex, scene_sampler, uv);
    let luma = dot(color.rgb, vec3<f32>(0.2126, 0.7152, 0.0722));
    return vec4<f32>(mix(color.rgb, vec3<f32>(luma), strength), color.a);
}
