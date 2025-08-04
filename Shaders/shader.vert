#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(set = 0, binding = 0) uniform MVPData
{
	mat4 model;
	mat4 view;
	mat4 proj;
} mvp;

layout(location = 0) out vec2 TexCoord;

void main()
{
	TexCoord = aTexCoord;
	gl_Position = mvp.proj * mvp.view * mvp.model * vec4(aPos, 1.0);
}