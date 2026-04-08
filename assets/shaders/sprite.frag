#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D image;
// uniform vec3 baseColor;

void main()
{
    color = texture(image, TexCoords);
}
