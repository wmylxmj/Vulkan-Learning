#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 texcoord;
layout (location = 2) in vec4 normal;
layout (location = 3) in vec4 tangent;

layout (location = 4) in vec4 color;

layout (binding = 0) uniform ub0 {
	mat4 ub0_modelMatrix;
    mat4 ub0_viewMatrix;
    mat4 ub0_projectionMatrix;
    mat4 ub0_normalMatrix;
	mat4 ub0_reserved[1020];
};

layout (binding = 1) uniform ub1 {
	mat4 ub1_modelMatrix;
    mat4 ub1_viewMatrix;
    mat4 ub1_projectionMatrix;
    mat4 ub1_normalMatrix;
	mat4 ub1_reserved[1020];
};

layout (location = 0) out vec4 v_color;

void main() {
	v_color = color;
	gl_Position = ub1_projectionMatrix * ub1_viewMatrix * ub0_modelMatrix * position;
}
