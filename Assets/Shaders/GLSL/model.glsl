#shader vertex

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoords;

layout(location = 0) out vec3 o_FragPosition;
layout(location = 1) out vec3 o_Normal;
layout(location = 2) out vec2 o_TexCoords;

layout(binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} u_Matrices;
layout(binding = 1) uniform Model {
    mat4 model;
} u_Model;

void main()
{
    o_FragPosition = vec3(u_Model.model * vec4(a_Position, 1.0));
    o_TexCoords = a_TexCoords;
    o_Normal = a_Normal;
    gl_Position = u_Matrices.projection * u_Matrices.view * u_Model.model * vec4(a_Position, 1.0);
}

#shader fragment

layout(location = 0) in vec3 FragPosition;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

layout(location = 0) out vec4 SceneOutput;

layout(binding = 0) uniform sampler2D TextureDiffuse;

layout(binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} u_Matrices;

struct Light
{
    vec4 position;
    vec4 direction;

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    int type;
};
layout(std140, binding = 2) uniform LightBlock {
    Light lights[32];
    int numLights;
} lightBlock;

layout(std140, binding = 3) uniform Material
{
    float shininess;
    float padding[3];
} material;

vec3 CalculateLightContrib(Light light, vec3 baseColor, vec3 specularColor)
{
    vec3 ambient = light.ambient.rgb;
    vec3 diffuse = light.diffuse.rgb;
    vec3 specular = light.specular.rgb;
    vec3 result = vec3(0.0);
    float attenuation = 1.0;
    float intensity = 1.0;
    vec3 lightDir = normalize(light.position.rgb - FragPosition);
    switch (light.type)
    {
    // point
    case 0:
    {
        float distance = length(light.position.rgb - FragPosition);
        attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    }
    break;
    // spot
    case 1:
    {
        float theta = dot(lightDir, normalize(-light.direction.rgb));
        float epsilon = max(light.cutOff - light.outerCutOff, 0.0001);
        intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

        // attenuation
        float distance = length(light.position.rgb - FragPosition);
        attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    }
    break;
    // area
    case 2:
    {
        lightDir = vec3(0.0, 0.0, 1.0);
    }
    break;
    // directional
    case 3:
    {
        lightDir = normalize(-light.direction.rgb);
    }
    break;
    }

    ambient = ambient * baseColor;

    // diffuse 
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    diffuse = diffuse * diff * baseColor;

    // specular
    vec3 viewDir = normalize(u_Matrices.viewPos.rgb - FragPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    specular = specular * spec * specularColor;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    diffuse *= intensity;
    specular *= intensity;

    result = ambient + diffuse + specular;
    return result;
}

void main()
{
    vec3 baseColor = texture(TextureDiffuse, TexCoords).rgb;

    vec3 lightContrib = vec3(0.0);
    vec3 result = vec3(0.0);

    int maxLights = min(lightBlock.numLights, 32);
    for (int i = 0; i < 3; i++)
    {
        lightContrib += CalculateLightContrib(lightBlock.lights[i], baseColor, vec3(1.0));
    }
    result += lightContrib;
    SceneOutput = vec4(result, 1.0);
}