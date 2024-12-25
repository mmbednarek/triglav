[[vk::binding(0, 0)]]
uniform RWTexture2D<float4> outTexture;

[numthreads(16, 16, 1)]
void cs_main(uint3 threadID : SV_DispatchThreadID)
{
    outTexture[int2(threadID.xy)] = ((threadID.x + threadID.y) % 2) == 0 ? float4(1, 0, 1, 1) : float4(1, 1, 0, 1);
}