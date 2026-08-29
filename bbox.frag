#version 450 core
layout(location = 0) in vec3 inColor;

layout(location = 1) uniform float alpha;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(inColor, alpha);
}