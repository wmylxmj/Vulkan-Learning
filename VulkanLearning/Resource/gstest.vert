#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 texcoord;
layout (location = 2) in vec4 normal;
layout (location = 3) in vec4 tangent;

layout (push_constant) uniform PushConstant {
	mat4 viewMatrix;
	mat4 projectionMatrix;
};

layout (binding = 0) uniform ub0 {
	mat4 ub0_modelMatrix;
    mat4 ub0_normalMatrix;
	mat4 ub0_reserved[1022];
};

layout (binding = 1) uniform ub1 {
	mat4 ub1_reserved[1024];
};

layout (location = 0) out vec4 v_texcoord;
layout (location = 1) out vec4 v_normalWorldSpace;
layout (location = 2) out vec4 v_positionWorldSpace;

void main() {
	v_normalWorldSpace = ub0_normalMatrix * normal;
	v_texcoord = texcoord;
	vec4 positionMS = position;
    v_positionWorldSpace = ub0_modelMatrix * positionMS;
	gl_Position = projectionMatrix * viewMatrix * v_positionWorldSpace;
}
