#version 430

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 texcoord;
layout (location = 2) in vec4 normal;
layout (location = 3) in vec4 tangent;

layout (location = 0) out vec4 v_texcoord;


void main()
{
	v_texcoord = texcoord;
    gl_Position = position;
}