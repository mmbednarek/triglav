import triglav.ui;

[[vk::binding(0)]]
uniform ConstantBuffer<uint> g_newPrimitiveCount;

[[vk::binding(1)]]
StructuredBuffer<triglav::ui::RectWriteData> g_writeOperations : register(t0);

[[vk::binding(2)]]
RWStructuredBuffer<triglav::ui::RectDrawCall> g_drawCallBuffer : register(u0);

[numthreads(256, 1, 1)]
void cs_main(uint tid: SV_DispatchThreadID)
{
    if (tid >= g_newPrimitiveCount)
        return;

    triglav::ui::RectWriteData rect = g_writeOperations[tid];

    triglav::ui::RectDrawCall drawCall;
    drawCall.vertexCount = 6;
    drawCall.instanceCount = 1;
    drawCall.firstInstance = 0;
    drawCall.firstVertex = 0;
    drawCall.primitive = rect.primitive;
    g_drawCallBuffer[rect.dstIndex] = drawCall;
}
