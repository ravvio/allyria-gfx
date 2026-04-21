#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coords;

uniform mat4 t_model;
uniform mat4 t_view;
uniform mat4 t_projection;

out vec2 uv;

void main() {
    gl_Position = t_projection * t_view * t_model * vec4(position.xy, 0.0, 1.0);
    uv = tex_coords;
}
