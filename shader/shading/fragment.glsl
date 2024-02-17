#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(binding = 0) uniform sampler2D texColor;
layout(binding = 1) uniform sampler2D texPosition;
layout(binding = 2) uniform sampler2D texNormal;
layout(binding = 3) uniform sampler2D texAmbientOcclusion;
layout(binding = 4) uniform sampler2D texShadowMap;

layout(binding = 5) uniform PostProcessingUBO {
    mat4 shadowMapMat;
} ubo;

layout(location = 0) out vec4 outColor;

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
        if (shadowCoord.w > 0.0 && dist < shadowCoord.z - 0.001)
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
    int range = 2;

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

const float ambient = 0.5;

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

const float pi = 3.14159265;

float distribution_ggx(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denominator = NdotH * NdotH * (alphaSq - 1.0) + 1.0;
    denominator = pi * denominator * denominator;
    return alphaSq / max(denominator, 0.000001);
}

float smith_formula(float NdotV, float NdotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float HdotV, vec3 baseReflectivity) {
    return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - HdotV, 5.0);
}

const float g_gaussCoefs[] = {
7.399735821039532e-06,
0.0010982181697249752,
0.022058301597726565,
0.05996068039976925,
0.022058301597726565,
0.0010982181697249752,
7.399735821039532e-06,
9.769077867254032e-06,
0.0014498597077982306,
0.02912121069442344,
0.07915965785337846,
0.02912121069442344,
0.0014498597077982306,
9.769077867254032e-06,
1.154080186257586e-05,
0.0017128068630017834,
0.034402645489111545,
0.09351608608397045,
0.034402645489111545,
0.0017128068630017834,
1.154080186257586e-05,
1.2200101845709555e-05,
0.00181065565629493,
0.036367991039690406,
0.09885844918075183,
0.036367991039690406,
0.00181065565629493,
1.2200101845709555e-05,
1.154080186257586e-05,
0.0017128068630017834,
0.034402645489111545,
0.09351608608397045,
0.034402645489111545,
0.0017128068630017834,
1.154080186257586e-05,
9.769077867254032e-06,
0.0014498597077982306,
0.02912121069442344,
0.07915965785337846,
0.02912121069442344,
0.0014498597077982306,
9.769077867254032e-06,
7.399735821039532e-06,
0.0010982181697249752,
0.022058301597726565,
0.05996068039976925,
0.022058301597726565,
0.0010982181697249752,
7.399735821039532e-06
};

float sample_ao_blurred(vec2 coord) {
    ivec2 texSize = textureSize(texAmbientOcclusion, 0);
    vec2 pixelOffset = vec2(1.0 / texSize.x, 1.0 / texSize.y);

    float result = 0.0;
    int coefIndex = 0;
    for (int y = -3; y <= 3; ++y) {
        for (int x = -3; x <= 3; ++x) {
            float coef = g_gaussCoefs[coefIndex];
            result += coef * texture(texAmbientOcclusion, coord + vec2(x * pixelOffset.x, y * pixelOffset.y)).r;
            ++coefIndex;
        }
    }

    return result;
}

void main() {
    vec3 normal = texture(texNormal, fragTexCoord).rgb;
    if (normal == vec3(0.0, 0.0, 0.0)) {
        outColor = vec4(texture(texColor, fragTexCoord).rgb, 1.0);
        return;
    }
    normal = normalize(normal);

    vec4 texPositionSample = texture(texPosition, fragTexCoord);
    vec3 position = texPositionSample.xyz;
    vec4 shadowUV = biasMat * ubo.shadowMapMat * vec4(position, 1.0);
    shadowUV /= shadowUV.w;

    float shadow = filterPCF(shadowUV);
    //    float ambientValue = ambient * texture(texAmbientOcclusion, fragTexCoord).r;
    float ambientValue = ambient;
    if (pc.enableSSAO) {
        ambientValue *= sample_ao_blurred(fragTexCoord);
//        outColor = vec4(vec3(sample_ao_blurred(fragTexCoord)), 1.0);
//        return;
    }
//    outColor = vec4(vec3(ambientValue), 1.0);
//    return;

    vec4 texColorSample = texture(texColor, fragTexCoord);
    vec3 albedo = texColorSample.rgb;
    albedo = pow(albedo, vec3(1.3));

    const float roughness = texColorSample.w;
    const float metallic = texPositionSample.w;

    vec3 viewDir = normalize(-position);
    vec3 baseReflectivity = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0);
    // PER LIGHT
    vec3 lightDir = normalize(pc.lightPosition - position);
    vec3 halfpoint = normalize(viewDir + lightDir);
    float attenuation = 4.0;
    vec3 radience = vec3(1, 1, 1) * attenuation;

    float NdotV = max(dot(normal, viewDir), 0.00000001);
    float NdotL = max(dot(normal, lightDir), 0.00000001);
    float HdotV = max(dot(halfpoint, viewDir), 0);
    float NdotH = max(dot(normal, halfpoint), 0);

    float dist = distribution_ggx(NdotH, roughness);
    float geo = smith_formula(NdotV, NdotL, roughness);
    vec3 fresnel = fresnel_schlick(HdotV, baseReflectivity);

    vec3 specular = dist * geo * fresnel;
    specular /= 4.0 * NdotV * NdotL;

    vec3 kD = vec3(1.0) - fresnel;
    kD *= 1.0 - metallic;

    Lo += (kD * albedo / pi + specular) * radience * NdotL;

    // END PER LIGHT

    vec3 ambient = albedo * ambientValue;
    vec3 color = ambient + shadow * Lo;
    //    color = color / (color + vec3(1.0));
    //    color = pow(color, vec3(1.3));

    float luminance = dot(color, vec3(0.2125, 0.7153, 0.07121));
    color = mix(vec3(luminance), color, 1.2);

    outColor = vec4(color, 1.0);
}