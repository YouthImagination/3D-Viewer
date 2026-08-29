#version 450 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in ivec4 inBoneIds;
layout(location = 4) in vec4 inWeights;
layout(location = 5) in vec3 inTangent; // Vertex tangent

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoords;
layout(location = 2) out vec3 outFragPos;
layout(location = 3) out mat3 outTBN; // Tangent Space TBN matrix

layout(location = 0) uniform mat4 model;

layout(std140, binding = 1) uniform MatricesBlock {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} scene;

// Dynamic Bone Transforms updated via AZDO SSBO
layout(std430, binding = 0) buffer BoneBlock {
    mat4 boneTransforms[];
};

void main() {
    mat4 boneTransform = mat4(0.0);
    for(int i = 0; i < 4; i++) {
        if(inBoneIds[i] >= 0) {
            boneTransform += boneTransforms[inBoneIds[i]] * inWeights[i];
        }
    }
    if(boneTransform == mat4(0.0)) {
        boneTransform = mat4(1.0);
    }
    
    vec4 localPosition = boneTransform * vec4(inPos, 1.0);
    gl_Position = scene.projection * scene.view * model * localPosition;
    
    outNormal = normalize(mat3(model * boneTransform) * inNormal);
    outFragPos = vec3(model * localPosition);
    // Flip texture V-coordinate for OpenGL relative to glTF convention
    outTexCoords = vec2(inTexCoords.x, 1.0 - inTexCoords.y);

    // Compute tangent space TBN matrix
    vec3 T = normalize(mat3(model * boneTransform) * inTangent);
    vec3 N = outNormal;
    // Gram-Schmidt re-orthogonalization
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    outTBN = mat3(T, B, N);
}
