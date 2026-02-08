import triglav.ui;

[[vk::binding(0)]]
uniform ConstantBuffer<uint> g_rectRemovalCount;

struct RectCopyInfo
{
    uint srcID;
    uint dstID;
};
[[vk::binding(1)]]
StructuredBuffer<RectCopyInfo> g_rectRemovalIDs : register(t0);

[[vk::binding(2)]]
RWStructuredBuffer<triglav::ui::RectDrawCall> g_drawCallBuffer : register(u0);

[numthreads(256, 1, 1)]
void cs_main(uint tid: SV_DispatchThreadID)
{
    if (tid >= g_rectRemovalCount)
        return;

    const RectCopyInfo copyInfo = g_rectRemovalIDs[tid];
    g_drawCallBuffer[copyInfo.dstID] = g_drawCallBuffer[copyInfo.srcID];
}
