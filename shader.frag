#version 450 core
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 inFragPos;
layout(location = 3) in mat3 inTBN; // Tangent Space TBN matrix

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D metallicRoughnessTex;

layout(std140, binding = 1) uniform MatricesBlock {
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
} scene;

layout(location = 4) uniform vec3 lightDir;
layout(location = 5) uniform vec3 lightColor;
layout(location = 6) uniform int hasTexture;
layout(location = 7) uniform int hasNormalMap;
layout(location = 8) uniform int hasMetallicRoughnessMap;
layout(location = 9) uniform float ambientIntensity;

const float PI = 3.14159265359;

// GGX Normal Distribution Function (NDF)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / max(denom, 0.0000001);
}

// Schlick Geometry G1
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

// Smith Geometry Function (G)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx2 * ggx1;
}

// Fresnel Specular Factor (F)
vec3 fresnelSchlick(float MathCosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - MathCosTheta, 0.0, 1.0), 5.0);
}

// Cosine-based rainbow palette generator
vec3 rainbow(float factor) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.0, 0.33, 0.67);
    return a + b * cos(2.0 * PI * (c * factor + d));
}

// Analytical thin-film interference simulation (OPD to color mapping)
vec3 calculateIridescenceRainbow(float MathCosTheta, float IOR, float thickness) {
    float sinTheta2 = (1.0 / IOR) * sqrt(max(0.0, 1.0 - MathCosTheta * MathCosTheta));
    float cosTheta2 = sqrt(max(0.0, 1.0 - sinTheta2 * sinTheta2));
    
    // Optical Path Difference (OPD)
    float OPD = 2.0 * IOR * thickness * cosTheta2;
    
    // Map OPD to rainbow factor
    float factor = OPD / 800.0;
    return rainbow(factor);
}

void main() {
    vec3 albedo = vec3(0.9, 0.7, 0.2); // Golden-yellow bee fallback
    float alpha = 1.0;
    
    if (hasTexture != 0) {
        vec4 texColor = texture(diffuseTex, inTexCoords);
        albedo = texColor.rgb;
        alpha = texColor.a;
    }
    
    // Normal mapping
    vec3 N;
    if (hasNormalMap != 0) {
        // re-mapping into vector
        vec3 normalMapSample = texture(normalTex, inTexCoords).rgb;
        normalMapSample = normalMapSample * 2.0 - 1.0;
        N = normalize(inTBN * normalMapSample);
    } else {
        N = normalize(inNormal);
    }
    
    vec3 V = normalize(scene.cameraPos - inFragPos);
    vec3 L = normalize(lightDir);
    vec3 H = normalize(V + L);

    // Metallic-Roughness mapping (glTF: Green = Roughness, Blue = Metallic, Red = Occlusion/AO)
    float roughness = 0.5;
    float metallic = 0.0;
    float ao = 1.0;
    if (hasMetallicRoughnessMap != 0) {
        vec4 mrSample = texture(metallicRoughnessTex, inTexCoords);
        ao = mrSample.r;
        roughness = mrSample.g;
        metallic = mrSample.b;
    }

    // --- Lighting model matching Windows 3D Viewer ---

    // 1. Hemisphere ambient: blend between a cool sky color and warm ground color
    //    This prevents pure-black shadows and gives a natural outdoor feel
    float hemiBlend = 0.5 * (N.y + 1.0); // 0 = down-facing, 1 = up-facing
    vec3 skyColor  = vec3(0.85, 0.90, 1.0);  // Cool blue-ish sky
    vec3 groundColor = vec3(0.6, 0.55, 0.5); // Warm brownish ground bounce
    vec3 hemiAmbient = mix(groundColor, skyColor, hemiBlend);
    float ambientBase = max(ambientIntensity, 0.25); // floor to prevent too-dark scenes
    vec3 ambient = ambientBase * hemiAmbient * albedo * ao;

    // 2. Diffuse (directional key light)
    float diff = max(dot(N, L), 0.0);
    // Wrap lighting: soften the terminator to reduce harsh light/dark boundary
    float wrapDiff = max((dot(N, L) + 0.3) / 1.3, 0.0);
    vec3 diffuse = wrapDiff * lightColor;

    // 3. Secondary Fill Light (opposite direction, elevated, strong enough to reveal shadow detail)
    vec3 fillL = normalize(vec3(-L.x, 0.6, -L.z));
    float fillDiff = max((dot(N, fillL) + 0.3) / 1.3, 0.0);
    vec3 fillDiffuse = fillDiff * (lightColor * 0.45);

    // 4. Specular (Blinn-Phong) - wider, softer highlight to match 3D Viewer
    float shininess = mix(16.0, 64.0, 1.0 - roughness); // roughness-dependent
    float specStrength = mix(0.3, 0.6, metallic); // metals get stronger spec
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = spec * vec3(specStrength) * lightColor;

    // 5. Fill light specular (subtle)
    vec3 fillH = normalize(V + fillL);
    float fillSpec = pow(max(dot(N, fillH), 0.0), shininess);
    vec3 fillSpecular = fillSpec * vec3(specStrength * 0.3) * lightColor;

    // Apply Thin-Film Iridescence to translucent regions (wings)
    if (alpha < 0.9) {
        float MathCosTheta = max(dot(N, V), 0.0);
        vec3 iridColor = calculateIridescenceRainbow(MathCosTheta, 1.5, 350.0);
        specular *= iridColor * 2.0;
        fillSpecular *= iridColor * 2.0;
    }

    // Combine: ambient + (diffuse + fill) * albedo + specular highlights
    vec3 color = ambient + (diffuse + fillDiffuse) * albedo + specular + fillSpecular;

    // Gentle tonemapping (ACES-like curve, less aggressive than Reinhard)
    // This preserves midtones and bright highlights better
    vec3 x = color;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    color = clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    // Standard alpha output for regular alpha blending
    fragColor = vec4(color, alpha);
}
