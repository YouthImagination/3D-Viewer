#version 450 core
layout(location = 0) in vec2 inTexCoords;
layout(location = 0) out vec4 fragColor;

layout(binding = 15) uniform sampler2D tex; // Isolated binding point to prevent unit 0 collision

void main() {
    vec4 color = texture(tex, inTexCoords);
    // Premultiply RGB by Alpha for front-to-back OIT blending
    fragColor = vec4(color.rgb * color.a, color.a);
}
