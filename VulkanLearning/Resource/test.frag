#version 430

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec4 InColor;

void main() {
	FragColor = InColor;
}

