
[[vk::binding(0)]]
[[vk::image_format("r16")]]
uniform Texture2D<float> InDepthPrepass;

[[vk::binding(1)]]
[[vk::image_format("r16")]]
uniform RWTexture2D<float> OutNextMip;

struct PushConstant
{
    int MipLevel;
};

[[vk::push_constant]]
ConstantBuffer<PushConstant> PC;

[numthreads(32, 32, 1)]
void cs_main(uint3 threadID : SV_DispatchThreadID)
{
    uint32_t width, height, levelCount;
    InDepthPrepass.GetDimensions(0, width, height, levelCount);

    uint2 sourcePos = 2*threadID.xy;

    if (sourcePos.x >= width || sourcePos.y >= height)
        return;

    let coord = int3(sourcePos.xy, PC.MipLevel);

    float samples[4] = {0, 0, 0, 0};
    samples[0] = InDepthPrepass.Load(coord);
    if (sourcePos.y < (height-1))
    {
        samples[1] = InDepthPrepass.Load(coord + int3(0, 1, 0));
    }
    if (sourcePos.x < (width-1))
    {
        samples[2] = InDepthPrepass.Load(coord + int3(1, 0, 0));
    }
    if (sourcePos.x < (width-1) && sourcePos.y < (height-1))
    {
        samples[3] = InDepthPrepass.Load(coord + int3(1, 1, 0));
    }

    const float maxSample = max(samples[0], max(samples[1], max(samples[2], samples[3])));

    OutNextMip[threadID.xy] = maxSample;
}