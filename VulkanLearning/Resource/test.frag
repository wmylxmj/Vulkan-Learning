#version 430

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_texcoord;
layout(location = 1) in vec4 v_normalWorldSpace;
layout (location = 2) flat in uint v_instanceID;


layout(binding = 2) uniform sampler2D u_texture[2];

void main() {
	vec3 color = texture(u_texture[v_instanceID], v_texcoord.xy).rgb;
	FragColor = vec4(color, 1.0);
}

