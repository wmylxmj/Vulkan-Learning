#version 430

layout(vertices = 4) out; // patch

void main() {
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    float splitCountU = 8.0;
    float splitCountV = 8.0;

    gl_TessLevelInner[0] = splitCountU; // 三角形内部 四边形u
    gl_TessLevelInner[1] = splitCountV; // 四边形v

    gl_TessLevelOuter[0] = splitCountV; // 三角形外部
    gl_TessLevelOuter[1] = splitCountU;
    gl_TessLevelOuter[2] = splitCountV;
    gl_TessLevelOuter[3] = splitCountU;
}