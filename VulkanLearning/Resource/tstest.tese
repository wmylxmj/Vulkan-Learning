#version 430

layout(triangles, equal_spacing, cw) in;

void main() {
    // ÖØÐÄ×ø±ê
	float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z;

    vec3 a = gl_in[0].gl_Position.xyz;
    vec3 b = gl_in[1].gl_Position.xyz;
    vec3 c = gl_in[2].gl_Position.xyz;
    gl_Position = vec4(a * u + b * v + c * w, 1.0);
}