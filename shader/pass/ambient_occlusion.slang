import "../common/common.slang";

[[vk::binding(0)]]
uniform Texture2D<float3> texPosition;
[[vk::binding(1)]]
uniform Texture2D<float3> texNormal;
[[vk::binding(2)]]
uniform Texture2D<float2> texNoise;

[[vk::binding(3)]]
uniform ConstantBuffer<common::ViewProperties> viewProps;

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

    float3 position = texPosition.Load(pixelPos);

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
        int3 offsetPos = int3(int2(offset.xy * float2(texDims.xy)), 0);

        if (offsetPos.x >= 0 && offsetPos.y >= 0 && offsetPos.x < texDims.x && offsetPos.y < texDims.y) {
            float3 occluderPos = texPosition.Load(offsetPos);
            float rangeCheck = smoothstep(0.0, 1.0, g_radius / length(position - occluderPos));
            occlusion += (occluderPos.z >= s.z + 0.1 ? rangeCheck : 0.0);
            ++good_samples;
        }
    }

    return float4(1.0 - (occlusion / float(good_samples)));
}
