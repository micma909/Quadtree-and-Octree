#version 460 core

uniform mat4 proj;
uniform mat4 view;

layout(location = 1) out vec3 nearPoint;
layout(location = 2) out vec3 farPoint;
out mat4 fragView;
out mat4 fragProj;

// Grid position are in xy clipped space
const vec3 quadVertices[4] = { vec3(-1.0, -1.0, 0.0), vec3(1.0, -1.0, 0.0), vec3(-1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0) };

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() 
{
	fragView = view;
	fragProj = proj;

	vec3 p = quadVertices[gl_VertexID].xyz;
	nearPoint = UnprojectPoint(p.x, p.y, 0.0, view, proj).xyz;
	farPoint  = UnprojectPoint(p.x, p.y, 1.0, view, proj).xyz;
	gl_Position = vec4(p, 1.0); 
}
