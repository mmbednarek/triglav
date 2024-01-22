#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texPosition;
layout(binding = 2) uniform sampler2D texNormal;
layout(binding = 3) uniform sampler2D texDepth;
layout(binding = 4) uniform sampler2D texNoise;
layout(binding = 5) uniform sampler2D texShadowMap;

layout(binding = 6) uniform PostProcessingUBO {
    mat4 shadowMapMat;
    mat4 cameraProjection;
    vec3 samplesSSAO[64];
} ubo;

layout(location = 0) out vec4 outColor;

const vec2 viewportSize = vec2(1280, 720);

layout(push_constant) uniform Constants
{
    vec3 lightPosition;
    bool enableSSAO;
} pc;

float linearize_depth(float depth)
{
    float n = 0.1;
    float f = 200.0;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

float textureProj(vec4 shadowCoord, vec2 off)
{
    float shadow = 1.0;
    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
        float dist = texture(texShadowMap, shadowCoord.st + off).r;
        if (shadowCoord.w > 0.0 && dist < shadowCoord.z - 0.00001)
        {
            shadow = 0;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc)
{
    ivec2 texDim = textureSize(texShadowMap, 0);
    float scale = 1.5;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
            count++;
        }

    }
    return shadowFactor / count;
}

const float ambient = 0.3;
const float shinniness = 50.0;

const float radius = 0.8;

const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0
);

vec4 visualizeVector(vec3 v) {
    return vec4(0.5 * v + 0.5, 1.0);
}

void main() {
    vec3 normal = texture(texNormal, fragTexCoord).rgb;
    if (normal == vec3(0.0, 0.0, 0.0)) {
        outColor = vec4(texture(texColor, fragTexCoord).rgb, 1.0);
        return;
    }
    normal = normalize(normal);

    vec4 position = texture(texPosition, fragTexCoord);
    vec4 shadowUV = biasMat * ubo.shadowMapMat * position;
    shadowUV /= shadowUV.w;

    float shadow = filterPCF(shadowUV);
    float ambientValue = ambient;

    // -------------------------------
    if (pc.enableSSAO) {
        vec4 randomPixel = texture(texNoise, fragTexCoord * vec2(1.0, 0.6));
        vec3 randomVector = normalize(randomPixel.xyz * 2 - 1);
        vec3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 ssaoTangentSpace = mat3(tangent, bitangent, normal);

        float occlusion = 0.0;
        for (int i = 0; i < 64; ++i) {
            vec3 s = ssaoTangentSpace * (ubo.samplesSSAO[i]);
            s = position.xyz + s * radius;

            vec4 offset = vec4(s, 1.0);
            offset = ubo.cameraProjection * offset;
            offset.xyz /= offset.w;
            offset.xyz = offset.xyz * 0.5 + 0.5;

            float sampleDepth = linearize_depth(offset.z);
            vec3 occluderPos = texture(texPosition, offset.xy).xyz;
            float rangeCheck = smoothstep(0.0, 1.0, radius / length(position.xyz - occluderPos));

            occlusion += (occluderPos.z >= s.z + 0.025 ? rangeCheck : 0.0);
        }

        ambientValue *= 1.0 - (occlusion / 64.0);
    }

    vec3 lightDir = normalize(pc.lightPosition - position.xyz);
    float diffuse = max(dot(normal, lightDir), 0);
    float specular = 0.0;
    if (diffuse > 0.0) {
        vec3 viewDir = normalize(-position.xyz);
        vec3 reflectDir = normalize(reflect(-lightDir, normal));
        specular = pow(max(dot(viewDir, reflectDir), 0.0), shinniness);
    }

    float lightValue = ambientValue + shadow * (diffuse + specular);

    outColor = vec4(lightValue * texture(texColor, fragTexCoord).rgb, 1.0);
}