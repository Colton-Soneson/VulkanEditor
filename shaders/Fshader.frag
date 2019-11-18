#version 440
#extension GL_ARB_seperate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 resolution;
layout(location = 2) in vec2 fragTextureCoord;
layout(location = 3) in float time;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(texSampler, fragTextureCoord);
}