import particle;
import triglav.core;

[[vk::binding(0)]]
ConstantBuffer<triglav::ViewProperties> ubo;

[[vk::binding(1)]]
StructuredBuffer<Particle> particlesIn;

static float2 g_positions[4] =
{
    float2(-1.0, -1.0),
    float2(-1.0, 1.0),
    float2(1.0, -1.0),
    float2(1.0, 1.0),
};

VertexOut vs_main(uint vertexIndex : SV_VertexID, uint instanceIndex : SV_InstanceID) {
    const float2 point = g_positions[vertexIndex];
    const Particle particle = particlesIn[instanceIndex];

    float sinA = sin(particle.rotation);
    float cosA = cos(particle.rotation);

    float3x3 mat = float3x3(cosA, sinA, 0, -sinA, cosA, 0, 0, 0, 1);

    float4 particleViewPos = mul(ubo.view, float4(particle.position, 1.0));
    particleViewPos.xy += mul(mat, particle.scale * float3(point, 1.0)).xy;

    int frameID = int(16.0  * particle.animation);
    int2 framePos = int2(frameID % 4, frameID / 4);

    VertexOut output;
    output.animation = particle.animation;
    output.texCoord = float2(0.25) * float2(framePos) + 0.25 * (float2(0.5) + 0.5*point);
    output.position = mul(ubo.proj, particleViewPos);

    return output;
}

[[vk::binding(2)]]
uniform Sampler2D texSampler;

struct FragmentOut
{
    [[vk::location(0)]]
    float4 color : SV_Target0;
    [[vk::location(1)]]
    float4 bloom : SV_Target1;
};

FragmentOut fs_main(FragmentIn fragIn)
{
    FragmentOut output;
    output.color = texSampler.Sample(fragIn.texCoord);
    output.color.rgb *= float3(1.0, 0.1, 0.1) * fragIn.animation + float3(1.0, 1.0, 0.1) * (1.0 - fragIn.animation);
    output.bloom = float4(0.3, 0.3, 0.3, 0.0) + output.color;
    return output;
}
