#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;
layout(location = 5) in vec4 fragShadowUV;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D shadowMapSampler;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Constants
{
    vec3 lightPosition;
} pc;

const vec3 black = vec3(0, 0, 0);
const vec3 white = vec3(1, 1, 1);
const float ambientStrength = 0.1;
const float shinniness = 60.0;

float textureProj(vec4 shadowCoord, vec2 off)
{
    float shadow = 1.0;
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
    {
        float dist = texture( shadowMapSampler, shadowCoord.st + off ).r;
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - 0.00001 )
        {
            shadow = 0;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc)
{
    ivec2 texDim = textureSize(shadowMapSampler, 0);
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

void main() {
    vec4 texPixel = texture(texSampler, fragTexCoord);
    vec4 shadowCoords = fragShadowUV / fragShadowUV.w;
    float shadow = filterPCF(shadowCoords);

    vec3 normal = 2 * texture(normalSampler, fragTexCoord).rgb - 1.0;
    vec3 normalVec = normalize(mat3( fragTangent, fragBitangent, fragNormal ) * normal);
    vec3 lightDir = normalize(pc.lightPosition - fragPosition);
    float diffuse = max(dot(normalVec, normalize(lightDir)), 0);
    float specular = 0.0;
    if (diffuse > 0.0) {
        vec3 viewVec = normalize(-fragPosition);
        vec3 halfVec = normalize(viewVec + lightDir);
        specular = pow(dot(halfVec, normalVec), shinniness);
    }

    float lightValue = ambientStrength + shadow * (diffuse + specular);
    vec3 result = lightValue * texPixel.rgb;
    outColor = vec4(result, 1);
}
