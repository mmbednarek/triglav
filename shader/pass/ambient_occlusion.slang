import triglav.core;

[[vk::binding(0)]]
uniform Texture2D<float> texDepth;
[[vk::binding(1)]]
uniform Texture2D<float3> texNormal;
[[vk::binding(2)]]
uniform Texture2D<float2> texNoise;

[[vk::binding(3)]]
uniform ConstantBuffer<triglav::ViewProperties> viewProps;

[[vk::binding(4)]]
uniform cbuffer SampleBuffer
{
    float3 g_samples[64];
};

static const float g_radius = 1.2;

float4 fs_main([[vk::location(0)]] float2 screenCoord : TEXCOORD0) : SV_Target0
{
    uint3 texDims;
    texNormal.GetDimensions(0, texDims.x, texDims.y, texDims.z);
    int3 pixelPos = int3(int2(screenCoord * float2(texDims.xy)), 0);

    float3 normal = texNormal.Load(pixelPos);
    if (all(normal == float3(0.0))) {
        return float4(1.0, 1.0, 1.0, 1.0);
    }

    normal = normalize(normal);
    normal = normalize(mul(float3x3(viewProps.view), normal));

    float3 position = viewProps.depth_view_pos(texDepth, screenCoord);

    uint3 noiseDims;
    texNoise.GetDimensions(0, noiseDims.x, noiseDims.y, noiseDims.z);
    int3 noisePos = int3(pixelPos.x % noiseDims.x, pixelPos.y % noiseDims.y, 0);

    float2 randomPixel = texNoise.Load(noisePos);
    float3 randomVector = normalize(float3(randomPixel * 2 - 1, 1));
    float3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 ssaoTangentSpace = transpose(float3x3(tangent, bitangent, normal));

    float occlusion = 0.0;
    int good_samples = 0;
    for (int i = 0; i < 64; ++i) {
        float3 s = mul(ssaoTangentSpace, g_samples[i]);
        s = position + s * g_radius;

        float4 offset = float4(s, 1.0);
        offset = mul(viewProps.proj, offset);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        if (offset.x >= 0 && offset.y >= 0 && offset.x < 1 && offset.y < 1) {
            float3 occluderPos = viewProps.depth_view_pos(texDepth, offset.xy);
            float rangeCheck = smoothstep(0.0, 1.0, g_radius / length(position - occluderPos));
            occlusion += (occluderPos.z >= s.z + 0.1 ? rangeCheck : 0.0);
            ++good_samples;
        }
    }

    return float4(1.0 - (occlusion / float(good_samples)));
}
