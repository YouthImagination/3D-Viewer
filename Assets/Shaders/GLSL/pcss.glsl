// Percentage Closer Soft Shadows (PCSS) 实现

// 光源深度贴图
uniform sampler2D u_ShadowMaps[4];
uniform sampler2D u_ShadowMapsPoint[4];

const float PI = 3.14159265359;
const float SHADOW_MAP_SIZE = 2048.0;
const float INVERSE_SHADOW_MAP_SIZE = 1.0 / SHADOW_MAP_SIZE;

// Poisson Disk 采样模式
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090121),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409164, 0.91437612),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
    );

// 生成随机数
float random(vec4 seed)
{
    float dot_product = dot(seed, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

// 计算可见性（阴影测试）
float shadowTest(sampler2D shadowMap, vec3 projCoords, float bias)
{
    if (projCoords.z > 1.0)
        return 1.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    return currentDepth - bias > closestDepth ? 0.0 : 1.0;
}

// 计算平均遮挡深度（PCF 滤波器根）
float computeAverageBlocker(sampler2D shadowMap, vec3 projCoords, float searchRadius, float texelSize)
{
    float blockerSum = 0.0;
    float blockerCount = 0.0;

    for (int i = 0; i < 16; ++i)
    {
        vec2 offset = poissonDisk[i] * searchRadius * texelSize;
        float depth = texture(shadowMap, projCoords.xy + offset).r;

        if (depth < projCoords.z)
        {
            blockerSum += depth;
            blockerCount += 1.0;
        }
    }

    if (blockerCount > 0.0)
    {
        return blockerSum / blockerCount;
    }

    return -1.0;  // 无遮挡物
}

// 计算半影大小
float computePenumbraSize(float blockerDepth, float fragDepth, float lightSize)
{
    if (blockerDepth < 0.0)
        return 0.0;

    return (fragDepth - blockerDepth) * lightSize / blockerDepth;
}

// PCSS 主函数
float pcss(sampler2D shadowMap, vec3 projCoords, vec3 fragPos,
    vec3 lightPos, float lightSize, float searchRadius, float filterRadius, float bias)
{
    if (projCoords.z > 1.0 || projCoords.z < 0.0)
        return 1.0;

    float texelSize = INVERSE_SHADOW_MAP_SIZE;

    // 第一步：计算平均遮挡深度
    float blockerDepth = computeAverageBlocker(shadowMap, projCoords, searchRadius, texelSize);

    if (blockerDepth < 0.0)
    {
        // 无遮挡物，完全光照
        return 1.0;
    }

    // 第二步：计算半影大小
    float penumbraSize = computePenumbraSize(blockerDepth, projCoords.z, lightSize);
    float filterRadiusPixels = penumbraSize * SHADOW_MAP_SIZE;

    // 第三步：使用计算的半影进行 PCF 滤波
    float visibility = 0.0;
    vec4 seed = vec4(fragPos, 1.0);

    for (int i = 0; i < 16; ++i)
    {
        vec2 offset = poissonDisk[i] * filterRadiusPixels * texelSize;
        visibility += shadowTest(shadowMap, projCoords + vec3(offset, 0.0), bias);
    }

    return visibility / 16.0;
}

// 方向光 PCSS
float pcssDirectional(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 fragPos,
    vec3 lightDir, float bias)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    const float LIGHT_SIZE = 0.01;  // 方向光相对大小
    const float SEARCH_RADIUS = 2.5;

    return pcss(shadowMap, projCoords, fragPos, -lightDir, LIGHT_SIZE, SEARCH_RADIUS, 2.5, bias);
}

// 点光源 PCSS
float pcssPoint(sampler2D shadowMap, vec3 fragPos, vec3 lightPos, float far, float bias)
{
    vec3 fragToLight = fragPos - lightPos;
    float dist = length(fragToLight);

    // 投影坐标
    vec3 projCoords = fragToLight.xyz;
    float currentDepth = (dist - 0.0) / far;

    const float LIGHT_SIZE = 0.05;
    const float SEARCH_RADIUS = 3.0;

    // 立方体贴图采样需要特殊处理
    float visibility = 0.0;
    for (int i = 0; i < 16; ++i)
    {
        vec3 offset = normalize(fragToLight) * poissonDisk[i].x * SEARCH_RADIUS * 0.1;
        vec3 sampleCoords = fragToLight + offset;
        float depth = texture(shadowMap, normalize(sampleCoords) * 0.5 + 0.5).r;

        if (currentDepth - bias > depth)
            visibility += 1.0;
    }

    return 1.0 - (visibility / 16.0);
}