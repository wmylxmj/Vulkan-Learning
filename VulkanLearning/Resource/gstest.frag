#version 430

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_texcoord;
layout(location = 1) in vec4 v_normalWorldSpace;
layout (location = 2) in vec4 v_positionWorldSpace;


layout(binding = 2) uniform sampler2D u_texture;
layout (binding = 3) uniform samplerCube u_cubeMap;

void main() {
	/*
	vec3 N = normalize(v_normalWorldSpace.xyz);
	vec3 V = normalize(v_positionWorldSpace.xyz - vec3(200.0, -200.0, 300.0));
	vec3 R = normalize(reflect(V, N));
	vec3 color = texture(u_cubeMap, R).rgb;
	*/
	vec3 color = normalize(v_normalWorldSpace.xyz) * 0.5 + 0.5;
	FragColor = vec4(color, 1.0);
}

