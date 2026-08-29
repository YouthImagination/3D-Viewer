#version 460 core

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inFragPos;
layout(location = 2) in flat int inMeshletID;

layout(location = 0) out vec4 fragColor;

layout(location = 1) uniform vec3 cameraPos;
layout(location = 2) uniform vec3 lightDir; // Direction of light source

// Generate distinct color per meshlet ID
vec3 hashColor(int id) {
    float r = fract(sin(float(id) * 12.9898) * 43758.5453);
    float g = fract(sin(float(id) * 78.233) * 43758.5453);
    float b = fract(sin(float(id) * 45.164) * 43758.5453);
    return vec3(r, g, b) * 0.4 + vec3(0.6); // Muted colors
}

void main() {
    // Clean gypsum/clay base color (removing meshlet debug colors)
    vec3 baseColor = vec3(0.85, 0.82, 0.78);
    
    // Blinn-Phong Light vectors
    vec3 N = normalize(inNormal);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(cameraPos - inFragPos);
    vec3 H = normalize(L + V); // Halfway vector
    
    // 1. Ambient Term
    vec3 ambient = vec3(0.18) * baseColor;
    
    // 2. Diffuse Term
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * baseColor * 0.75;
    
    // Secondary Fill Light (pointing from opposite direction to soften dark sides)
    vec3 fillL = normalize(vec3(-L.x, 1.0, -L.z));
    float fillDiff = max(dot(N, fillL), 0.0);
    vec3 fillDiffuse = fillDiff * baseColor * (0.75 * 0.35);
    
    // 3. Specular Term (Blinn-Phong)
    float spec = pow(max(dot(N, H), 0.0), 32.0); // Shininess = 32
    vec3 specular = spec * vec3(0.35); // Soft white/grey specular specular highlights
    
    vec3 finalColor = ambient + diffuse + fillDiffuse + specular;
    
    fragColor = vec4(finalColor, 1.0);
}
