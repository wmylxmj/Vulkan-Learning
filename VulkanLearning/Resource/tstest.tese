#version 430

layout(quads, equal_spacing, ccw) in;

void main() {
    // ÖØÐÄ×ø±ê
	float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z;

    vec3 lb = gl_in[0].gl_Position.xyz;
    vec3 rb = gl_in[1].gl_Position.xyz;
    vec3 lt = gl_in[2].gl_Position.xyz;
    vec3 rt = gl_in[3].gl_Position.xyz;

    vec3 lrt = mix(lt, rt, u);
    vec3 lrb = mix(lb, rb, u);
    vec3 p = mix(lrb, lrt, v);
    gl_Position = vec4(p, 1.0);
}