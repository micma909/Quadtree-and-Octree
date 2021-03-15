#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aOffset;
layout (location = 2) in vec3 aSize;
layout (location = 3) in vec4 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 fragCol;

void main() {
	vec3 pos = aPos * aSize;
	
	fragCol = aColor;
	//fragCol.w = 0.2f;
	
	gl_Position = projection * view * model * vec4(pos + aOffset, 1.0);
}