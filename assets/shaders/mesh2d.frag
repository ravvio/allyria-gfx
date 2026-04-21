#version 330 core
out vec4 color;

in vec2 uv;

void main() {
    color = vec4(uv.xy, 0.0, 1.0);
}
