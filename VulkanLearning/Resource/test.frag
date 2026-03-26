#version 430

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec4 InColor;

void main() {
	vec3 normal = normalize(vec3(InColor));
	vec3 color = normal * 0.5 + 0.5;
	FragColor = vec4(color, 1.0);
}

