#version 450 core
layout(location = 0) out vec2 outTexCoords;

void main() {
    // Procedurally generate a full-screen triangle covering [-1, 1]
    vec2 grid = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(grid * 2.0 - 1.0, 0.0, 1.0);
    outTexCoords = grid;
}
