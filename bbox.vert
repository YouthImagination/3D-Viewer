#version 450 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) uniform mat4 model;

layout(std140, binding = 1) uniform MatricesBlock {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} scene;

layout(location = 0) out vec3 outColor;

void main() {
    gl_Position = scene.projection * scene.view * model * vec4(inPos, 1.0);

    outColor = inColor;
}
