#version 330 core
out vec4 FragColor;

in vec2 uv;

struct Material {
    vec4 albedo;
    sampler2D albedoMap;
};
uniform Material material;

void main() {
    FragColor = material.albedo * texture(material.albedoMap, uv);
}
