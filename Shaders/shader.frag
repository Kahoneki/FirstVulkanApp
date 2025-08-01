#version 450

layout(location = 0) out vec4 outColour;
layout(set = 0, binding = 0) readonly buffer DataBuffer
{
	uint data[];
} dataBuffer;

void main()
{
	//First uint = 0xCDCDCDCD
	uint value = dataBuffer.data[0];
	if (value == 0xCDCDCDCD)
	{
		outColour = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else
	{
		outColour = vec4(1.0, 0.0, 1.0, 1.0);
	}
}