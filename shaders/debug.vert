#version 420 core

layout(location = 0) in vec2 position;
layout(location = 0) in vec3 color;

out vec3 fragmentColor;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(position, 0.0, 1.0);
	fragmentColor = color;
}