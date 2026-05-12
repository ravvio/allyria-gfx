#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoords;

uniform mat4 t_camera_mvp;
uniform mat4 t_model;

out vec2 TexCoords;

void main()
{
    TexCoords = texCoords;
    gl_Position = t_camera_mvp * t_model * vec4(position.xy, 0.0, 1.0);
}
