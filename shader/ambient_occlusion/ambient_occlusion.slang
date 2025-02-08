import "../common/common.slang";

struct SamplesBuffer
{
    float3 samples[64];
};

struct FragIn
{
    [[vk::location(0)]]
    float2 screenCoord : TEXCOORD0;
};

[[vk::binding(0)]]
uniform Sampler2D<float4> texPosition;
[[vk::binding(1)]]
uniform Sampler2D<float4> texNormal;
[[vk::binding(2)]]
uniform Sampler2D<float4> texNoise;
[[vk::binding(3)]]
uniform ConstantBuffer<common::ViewProperties> viewProps;
[[vk::binding(4)]]
uniform ConstantBuffer<SamplesBuffer> samples;

static const float radius = 1.2;

struct FragOut
{
    [[vk::location(0)]]
    float4 outValue : SV_Target0;
};

FragOut fs_main(const FragIn fragIn)
{
    float3 normal = texNormal.Sample(fragIn.screenCoord).xyz;
    if (all(normal == float3(0.0))) {
        return FragOut(float4(1.0, 1.0, 1.0, 1.0));
    }

    normal = normalize(normal);

    float3 position = texPosition.Sample(fragIn.screenCoord).xyz;

    float4 randomPixel = texNoise.Sample(fragIn.screenCoord * float2(1.0, 0.6));
    float3 randomVector = normalize(randomPixel.xyz * 2 - 1);
    float3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 ssaoTangentSpace = transpose(float3x3(tangent, bitangent, normal));

    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i) {
        float3 s = mul(ssaoTangentSpace, samples.samples[i]);
        s = position + s * radius;

        float4 offset = float4(s, 1.0);
        offset = mul(viewProps.proj, offset);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float3 occluderPos = texPosition.Sample(offset.xy).xyz;
        float rangeCheck = smoothstep(0.0, 1.0, radius / length(position - occluderPos));

        occlusion += (occluderPos.z >= s.z + 0.1 ? rangeCheck : 0.0);
    }

    return FragOut(float4(1.0 - (occlusion / 64.0)));
}
