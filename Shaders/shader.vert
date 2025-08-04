#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(set = 0, binding = 0) uniform CameraData
{
	mat4 view;
	mat4 proj;
} cameraData;

layout(push_constant) uniform ModelData
{
	mat4 model;
} modelData;

layout(location = 0) out vec2 TexCoord;

void main()
{
	TexCoord = aTexCoord;
	gl_Position = cameraData.proj * cameraData.view * modelData.model * vec4(aPos, 1.0);
}