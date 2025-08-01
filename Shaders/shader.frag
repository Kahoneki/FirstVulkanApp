#version 450

layout(location = 0) out vec4 outColour;
layout(set = 0, binding = 0) uniform DataBuffer
{
	vec4 colour;
} dataBuffer;

void main()
{
	outColour = dataBuffer.colour;
}