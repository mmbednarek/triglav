import "../common/common.slang";

struct FragInput
{
    float4 position : SV_Position;

    [[vk::location(0)]]
    float2 texCoord : TEXCOORD0;
};

static const float2 POSITIONS[4] = {
    float2(-1.0, -1.0),
    float2(-1.0, 1.0),
    float2(1.0, -1.0),
    float2(1.0, 1.0),
};

[shader("vertex")]
FragInput vs_main(uint vertexIndex: SV_VertexID)
{
    const float2 pos = POSITIONS[vertexIndex];

    FragInput result;
    result.position = float4(pos, 0.0, 1.0);
    result.texCoord = (float2(1) + pos) * 0.5;

    return result;
}

struct FragOutput
{
    [[vk::location(0)]]
    float4 color : SV_Target0;
    [[vk::location(1)]]
    float4 bloom : SV_Target1;
};

[[vk::binding(0)]]
uniform Sampler2D<float4> texColor;
[[vk::binding(1)]]
uniform Sampler2D<float4> texPosition;
[[vk::binding(2)]]
uniform Sampler2D<float4> texNormal;
[[vk::binding(3)]]
uniform Sampler2D<float> texAmbientOcclusion;
[[vk::binding(4)]]
uniform Sampler2D<float> texShadowMaps[3];
[[vk::binding(5)]]
uniform Sampler2D<float> texShadow;

struct UniformViewProperties
{
    float4x4 view;
    float4x4 proj;
    float4x4 orient;
    float nearPlane;
    float farPlane;
};
[[vk::binding(6)]]
uniform ConstantBuffer<UniformViewProperties> viewProperties;

struct ShadowMapMatrices
{
    float4x4 matrix[3];
};
[[vk::binding(7)]]
uniform ConstantBuffer<ShadowMapMatrices> shadowMapMatrices;

struct PushConstants
{
    float4 lightPosition;
    bool isAmbientOcclusionEnabled;
    bool shouldSampleShadows;
};
[[vk::push_constant]]
uniform ConstantBuffer<PushConstants> pushConstants;

static const float4x4 BIAS_MAT = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
);

static const float STATIC_AMBIENT_TERM = 0.5;
static const float SHADOW_MAP_BIAS_TERM = 0.008;

[shader("fragment")]
FragOutput fs_main(FragInput fragInput)
{
    FragOutput result;

    float3 normal = texNormal.Sample(fragInput.texCoord).xyz;
    float4 texColorSample = texColor.Sample(fragInput.texCoord);

    if (all(normal == float3(0.0, 0.0, 0.0))) {
        result.color = float4(texColorSample.rgb, 1.0);
        result.bloom = float4(0.0);
        return result;
    }

    normal = normalize(normal);

    float4 texPositionSample = texPosition.Sample(fragInput.texCoord);
    float3 position = texPositionSample.xyz;

    float4 shadowUV = mul(mul(BIAS_MAT, shadowMapMatrices.matrix[0]), float4(position, 1.0));
    shadowUV /= shadowUV.w;

    float shadow;
    if (pushConstants.shouldSampleShadows) {
        shadow = texShadow.Sample(fragInput.texCoord);
    } else if (common::shadow_map::test_bounds(shadowUV)) {
        shadow = common::shadow_map::test_pcr(texShadowMaps[0], shadowUV, SHADOW_MAP_BIAS_TERM);
        // texColorSample.rgb *= float3(1.0, 0.1, 0.1);
    } else {
        shadowUV = mul(mul(BIAS_MAT, shadowMapMatrices.matrix[1]), float4(position, 1.0));
        shadowUV /= shadowUV.w;
        if (common::shadow_map::test_bounds(shadowUV)) {
            shadow = common::shadow_map::test_pcr(texShadowMaps[1], shadowUV, SHADOW_MAP_BIAS_TERM);
            // texColorSample.rgb *= float3(0.1, 1.0, 0.1);
        } else {
            shadowUV = mul(mul(BIAS_MAT, shadowMapMatrices.matrix[2]), float4(position, 1.0));
            shadowUV /= shadowUV.w;
            shadow = common::shadow_map::test_pcr(texShadowMaps[2], shadowUV, SHADOW_MAP_BIAS_TERM);
            // texColorSample.rgb *= float3(0.1, 0.1, 1.0);
        }
    }

    float ambientValue = STATIC_AMBIENT_TERM;
    if (pushConstants.isAmbientOcclusionEnabled) {
        ambientValue *= pow(texAmbientOcclusion.Sample(fragInput.texCoord), 1.5);
    }

    float3 albedo = texColorSample.rgb;

    const float roughness = texColorSample.w;
    const float metallic = texPositionSample.w;

    float3 viewDir = normalize(-position);
    float3 baseReflectivity = lerp(float3(0.04), albedo, metallic);

    common::ShadingInfo shadingInfo;
    shadingInfo.normal = normal;
    shadingInfo.viewDir = viewDir;
    shadingInfo.position = position;
    shadingInfo.baseReflectivity = baseReflectivity;
    shadingInfo.albedo = albedo;
    shadingInfo.roughness = roughness;
    shadingInfo.metallic = metallic;

    float3 Lo = float3(0);

    float3 lightDir = normalize(mul(float3x3(viewProperties.view), float3(-1, 0.0, -0.4)));
    Lo += shadingInfo.calculate_directional_light(lightDir);

    Lo += shadingInfo.calculate_point_light(mul(viewProperties.view, pushConstants.lightPosition).xyz, float3(1, 0.6, 0.05), 24.0);

    float3 ambient = albedo * ambientValue;
    float3 color = 0.5 * ambient + 1.2 * shadow * Lo;

    float luminance = dot(color, float3(0.2125, 0.7153, 0.07121));
    color = lerp(float3(luminance), color, 1.25);
    result.color = float4(color, 1.0);
    result.bloom = float4(0.0);

    return result;
}
