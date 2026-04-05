#version 430

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_texcoord;
layout(location = 1) in vec4 v_normalWorldSpace;

layout(binding = 0) uniform sampler2D u_texture;

void main() {
	vec3 color = texture(u_texture, v_texcoord.xy).rgb;
	FragColor = vec4(color, 1.0);
}

