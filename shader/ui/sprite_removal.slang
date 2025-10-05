import sprite_api;

[[vk::binding(0)]]
uniform ConstantBuffer<uint> g_spriteRemovalCount;

struct SpriteCopyInfo
{
    uint srcID;
    uint dstID;
};
[[vk::binding(1)]]
StructuredBuffer<SpriteCopyInfo> g_spriteRemovalIDs : register(t0);

[[vk::binding(2)]]
RWStructuredBuffer<sprite_api::DrawCall> g_drawCallBuffer : register(u0);

[numthreads(256, 1, 1)]
void cs_main(uint tid: SV_DispatchThreadID)
{
    if (tid >= g_spriteRemovalCount)
        return;

    const SpriteCopyInfo copyInfo = g_spriteRemovalIDs[tid];
    g_drawCallBuffer[copyInfo.dstID] = g_drawCallBuffer[copyInfo.srcID];
}
