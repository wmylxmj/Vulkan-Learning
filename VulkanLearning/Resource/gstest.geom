#version 430

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

layout(location = 0) in vec4 v_texcoord[];
layout(location = 1) in vec4 v_normalWorldSpace[];
layout (location = 2) in vec4 v_positionWorldSpace[];

layout(location = 0) out vec4 g_texcoord;
layout(location = 1) out vec4 g_normalWorldSpace;
layout (location = 2) out vec4 g_positionWorldSpace;

void main() {
	vec3 n = normalize(v_normalWorldSpace[0].xyz + v_normalWorldSpace[1].xyz + v_normalWorldSpace[2].xyz);
	gl_Position = gl_in[0].gl_Position;
	g_texcoord = v_texcoord[0];
	g_normalWorldSpace = vec4(n, 0);
	g_positionWorldSpace = v_positionWorldSpace[0];
	EmitVertex();
	gl_Position = gl_in[1].gl_Position;
	g_texcoord = v_texcoord[1];
	g_normalWorldSpace = vec4(n, 0);
	g_positionWorldSpace = v_positionWorldSpace[1];
	EmitVertex();
	gl_Position = gl_in[2].gl_Position;
	g_texcoord = v_texcoord[2];
	g_normalWorldSpace = vec4(n, 0);
	g_positionWorldSpace = v_positionWorldSpace[2];
	EmitVertex();
	EndPrimitive();
}