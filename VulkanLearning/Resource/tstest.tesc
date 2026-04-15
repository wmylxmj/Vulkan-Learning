#version 430

layout(vertices = 3) out; // patch

void main() {
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    gl_TessLevelInner[0] = 2.0; // 三角形内部
    gl_TessLevelInner[1] = 1.0;

    gl_TessLevelOuter[0] = 2.0; // 三角形外部
    gl_TessLevelOuter[1] = 2.0;
    gl_TessLevelOuter[2] = 2.0;
    gl_TessLevelOuter[3] = 1.0;
}