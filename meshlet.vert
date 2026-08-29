#version 460 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outFragPos;
layout(location = 2) out flat int outMeshletID;

layout(location = 0) uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(inPos, 1.0);
    outNormal = inNormal;
    outFragPos = inPos;
    outMeshletID = int(gl_BaseInstance);
}
