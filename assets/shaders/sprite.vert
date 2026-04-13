#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoords;

out vec2 uv;

uniform mat4 model;
uniform mat4 projection;

void main()
{
    uv = texCoords;
    gl_Position = projection * view * model * vec4(vertex.xy, 0.0, 1.0);
}
