#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 texcoord;
layout (location = 2) in vec4 normal;
layout (location = 3) in vec4 tangent;

layout (location = 4) in vec4 color;

layout (location = 0) out vec4 v_color;

void main() {
	v_color = color;
	gl_Position = position;
}
