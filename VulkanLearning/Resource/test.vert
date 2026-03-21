#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 texcoord;
layout (location = 2) in vec4 normal;
layout (location = 3) in vec4 tangent;

layout (location = 4) in vec4 color;

layout (binding = 0) uniform x {
	mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 normalMatrix;
	mat4 reserved[1020];
};

layout (location = 0) out vec4 v_color;

void main() {
	v_color = color;
	gl_Position = modelMatrix * position;
}
