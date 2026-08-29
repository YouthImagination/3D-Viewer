#version 450 core
layout(location = 0) in vec2 inTexCoords;
layout(location = 0) out vec4 fragColor;

layout(binding = 15) uniform sampler2D tex;

void main() {
    // Direct output of accumulated texture for screen composition
    fragColor = texture(tex, inTexCoords);
}
