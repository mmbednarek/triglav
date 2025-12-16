import "../common/common.slang";

[[vk::binding(0)]]
uniform Texture2D<float4> texColor;
[[vk::binding(1)]]
uniform Texture2D<float4> texPosition;
[[vk::binding(2)]]
uniform Texture2D<float3> texNormal;
[[vk::binding(3)]]
uniform Texture2D<float> texAmbientOcclusion;
[[vk::binding(4)]]
uniform Sampler2D<float> texShadowMaps[3];
[[vk::binding(5)]]
uniform Texture2D<float> texShadow;

[[vk::binding(6)]]
uniform ConstantBuffer<common::ViewProperties> viewProperties;

[[vk::binding(7)]]
uniform cbuffer ShadowMapMatrixBuffer
{
    float4x4 g_shadowMapMatrices[3];
}

[[vk::push_constant]]
uniform cbuffer PushConstants
{
    float4 g_lightPosition;
    bool g_isAmbientOcclusionEnabled;
    bool g_shouldSampleShadows;
}

static const float4x4 BIAS_MAT = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
);

static const float STATIC_AMBIENT_TERM = 0.5;
static const float SHADOW_MAP_BIAS_TERM_CLOSE = 0.0015;
static const float SHADOW_MAP_BIAS_TERM_MID = 0.002;
static const float SHADOW_MAP_BIAS_TERM_FAR = 0.004;

struct FragOutput
{
    [[vk::location(0)]]
    float4 color : SV_Target0;
    [[vk::location(1)]]
    float4 bloom : SV_Target1;
};

[shader("fragment")]
FragOutput fs_main([[vk::location(0)]] float2 screenCoord : TEXCOORD0)
{
    uint3 texDims;
    texColor.GetDimensions(0, texDims.x, texDims.y, texDims.z);
    int3 pixelPos = int3(int2(screenCoord * float2(texDims.xy)), 0);

    float3 normal = texNormal.Load(pixelPos);
    float4 texColorSample = texColor.Load(pixelPos);

    if (all(normal == float3(0.0, 0.0, 0.0))) {
        FragOutput result;
        result.color = float4(texColorSample.rgb, 1.0);
        result.bloom = float4(0.0);
        return result;
    }

    normal = normalize(normal);

    float4 texPositionSample = texPosition.Load(pixelPos);
    float3 position = texPositionSample.xyz;

    float4 shadowUV = mul(mul(BIAS_MAT, g_shadowMapMatrices[0]), float4(position, 1.0));
    shadowUV /= shadowUV.w;

    float shadow;
    if (g_shouldSampleShadows) {
        shadow = texShadow.Load(pixelPos);
    } else if (common::shadow_map::test_bounds(shadowUV)) {
        shadow = common::shadow_map::test_pcr(texShadowMaps[0], shadowUV, SHADOW_MAP_BIAS_TERM_CLOSE);
#if SHOW_SHADOW_CASCADES
        texColorSample.rgb *= float3(1.0, 0.1, 0.1);
#endif
    } else {
        shadowUV = mul(mul(BIAS_MAT, g_shadowMapMatrices[1]), float4(position, 1.0));
        shadowUV /= shadowUV.w;
        if (common::shadow_map::test_bounds(shadowUV)) {
            shadow = common::shadow_map::test_pcr(texShadowMaps[1], shadowUV, SHADOW_MAP_BIAS_TERM_MID);
#if SHOW_SHADOW_CASCADES
            texColorSample.rgb *= float3(0.1, 1.0, 0.1);
#endif
        } else {
            shadowUV = mul(mul(BIAS_MAT, g_shadowMapMatrices[2]), float4(position, 1.0));
            shadowUV /= shadowUV.w;
            shadow = common::shadow_map::test_pcr(texShadowMaps[2], shadowUV, SHADOW_MAP_BIAS_TERM_FAR);
#if SHOW_SHADOW_CASCADES
            texColorSample.rgb *= float3(0.1, 0.1, 1.0);
#endif
        }
    }

    float ambientValue = STATIC_AMBIENT_TERM;
    if (g_isAmbientOcclusionEnabled) {
        ambientValue *= pow(texAmbientOcclusion.Load(pixelPos), 1.5);
    }

    float3 albedo = texColorSample.rgb;

    const float roughness = texColorSample.w;
    const float metallic = texPositionSample.w;
    // const float roughness = texPositionSample.w;
    // const float metallic = texColorSample.w;

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

    Lo += shadingInfo.calculate_point_light(mul(viewProperties.view, g_lightPosition).xyz, float3(1, 0.6, 0.05), 24.0);

    float3 ambient = albedo * ambientValue;
    float3 color = 0.5 * ambient + 1.2 * shadow * Lo;

    float luminance = dot(color, float3(0.2125, 0.7153, 0.07121));
    color = lerp(float3(luminance), color, 1.25);

    FragOutput result;
    result.color = float4(color, 1.0);
    result.bloom = float4(0.0);
    return result;
}
