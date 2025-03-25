import text_processing;

[[vk::binding(0)]]
uniform ConstantBuffer<uint> g_textRemovalCount;

struct TextCopyInfo
{
    uint srcID;
    uint dstID;
};
[[vk::binding(1)]]
uniform StructuredBuffer<TextCopyInfo> g_textRemovalIDs;

[[vk::binding(2)]]
uniform RWStructuredBuffer<text_processing::DrawCall> g_drawCalls;

[numthreads(256, 1, 1)]
void cs_main(uint3 threadID: SV_GroupThreadID)
{
    if (threadID.x >= g_textRemovalCount)
        return;

    let copyInfo = g_textRemovalIDs[threadID.x];
    g_drawCalls[copyInfo.dstID] = g_drawCalls[copyInfo.srcID];
}
