#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 texcoord;
layout (location = 2) in vec4 normal;
layout (location = 3) in vec4 tangent;

layout (binding = 0) uniform ub0 {
	mat4 ub0_modelMatrix;
    mat4 ub0_viewMatrix;
    mat4 ub0_projectionMatrix;
    mat4 ub0_normalMatrix;
	mat4 ub0_reserved[1020];
};

layout (location = 0) out vec4 v_color;

void main() {
	v_color = position;
	gl_Position = ub0_projectionMatrix * ub0_viewMatrix * ub0_modelMatrix * position;
}
