#version 460 core

layout(location = 0) out vec4 outColor;

layout(location = 1) in vec3 nearPoint; 
layout(location = 2) in vec3 farPoint; 

in mat4 fragView;
in mat4 fragProj;

float near = 0.01f;
float far = 100.0f;

vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1)*20.0f;
    float minimumx = min(derivative.x, 1)*20.0f;
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1));
    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
        color.z = 1.0;
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
        color.x = 1.0;
    return color;
}
float computeDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
	float ndc_depth = (clip_space_pos.z / clip_space_pos.w);
    return (1.0 - 0.0) * 0.5 * ndc_depth + (1.0 + 0.0) * 0.5;
}
float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near)); // get linear value between 0.01 and 100
    return linearDepth / far; // normalize
}
void main() 
{	
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);

	vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

	gl_FragDepth = computeDepth(fragPos3D);
	
    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0, (1 - linearDepth));

    outColor = (grid(fragPos3D, 1) + grid(fragPos3D, 0.1)) ; // adding multiple resolution for the grid
    outColor.a *= fading*0.3f;

}