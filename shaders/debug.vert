#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

out vec4 fragmentColor;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP *  vec4(position, 1);// vec4(position, 0.0, 1.0);
	fragmentColor = color; //vec4(color,1);
}