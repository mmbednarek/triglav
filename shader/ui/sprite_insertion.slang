import triglav.ui;

[[vk::binding(0)]]
uniform ConstantBuffer<uint> g_newPrimitiveCount;

[[vk::binding(1)]]
StructuredBuffer<triglav::ui::SpriteWriteData> g_writeOperations : register(t0);

[[vk::binding(2)]]
RWStructuredBuffer<triglav::ui::SpriteDrawCall> g_drawCallBuffer : register(u0);

[numthreads(256, 1, 1)]
void cs_main(uint tid: SV_DispatchThreadID)
{
    if (tid >= g_newPrimitiveCount)
        return;

    triglav::ui::SpriteWriteData sprite = g_writeOperations[tid];

    triglav::ui::SpriteDrawCall drawCall;
    drawCall.vertexCount = 6;
    drawCall.instanceCount = 1;
    drawCall.firstInstance = 0;
    drawCall.firstVertex = 0;
    drawCall.primitive = sprite.primitive;
    g_drawCallBuffer[sprite.dstIndex] = drawCall;
}
