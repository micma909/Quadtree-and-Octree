#version 150 core

in vec3 fragmentColor;
out vec4 outColor;

void main()
{
    outColor = fragmentColor;
}