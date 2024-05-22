#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;
layout(location = 5) in vec3 fragWorldPosition;
layout(location = 6) in vec3 fragWorldNormal;
layout(location = 7) in vec3 fragWorldTangent;
layout(location = 8) in vec3 fragWorldBitangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D heightMapSampler;

layout(binding = 4) uniform MaterialProperties
{
    float roughness;
    float metallic;
    float heightScale;
} mp;

layout(push_constant) uniform Constants
{
    vec3 viewPos;
} pc;

vec2 offset_parallax(vec2 texCoords, vec3 viewDir)
{
    // number of depth layers
    const float minLayers = 4;
    const float maxLayers = 16;

    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;

    vec2 shift = viewDir.xy / viewDir.z * mp.heightScale;
    vec2 deltaTexCoords = shift / numLayers;

    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = 1 - texture(heightMapSampler, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = 1 - texture(heightMapSampler, currentTexCoords).r;
        // get depth of next layer
        currentLayerDepth += layerDepth;
    }

    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1 - texture(heightMapSampler, prevTexCoords).r) - currentLayerDepth + layerDepth;

    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main() {
    const mat3 tangentSpaceWorldMat = mat3(fragWorldTangent, fragWorldBitangent, fragWorldNormal);

    vec3 viewDir = normalize(tangentSpaceWorldMat * normalize(fragWorldPosition - pc.viewPos));
    vec2 parallaxUV = offset_parallax(fragTexCoord, viewDir);

    outColor = vec4(texture(texSampler, parallaxUV).rgb, mp.roughness);
    outPosition = vec4(fragPosition, mp.metallic);

    const mat3 tangentSpaceMat = mat3(fragTangent, fragBitangent, fragNormal);
    vec3 normalSample = 2 * texture(normalSampler, parallaxUV).rgb - 1.0;
    outNormal = vec4(normalize(tangentSpaceMat * normalSample), 1.0);
}
