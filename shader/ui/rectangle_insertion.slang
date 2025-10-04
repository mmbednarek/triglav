import rectangle_api;

[[vk::binding(0)]]
uniform ConstantBuffer<uint> g_newPrimitiveCount;

[[vk::binding(1)]]
StructuredBuffer<rectangle_api::WriteData> g_writeOperations : register(t0);

[[vk::binding(2)]]
RWStructuredBuffer<rectangle_api::DrawCall> g_drawCallBuffer : register(u0);

[numthreads(256, 1, 1)]
void cs_main(uint tid: SV_DispatchThreadID)
{
    if (tid >= g_newPrimitiveCount)
        return;

    rectangle_api::WriteData rect = g_writeOperations[tid];

    rectangle_api::DrawCall drawCall;
    drawCall.vertexCount = 6;
    drawCall.instanceCount = 1;
    drawCall.firstInstance = 0;
    drawCall.firstVertex = 0;
    drawCall.primitive = rect.primitive;
    g_drawCallBuffer[rect.dstIndex] = drawCall;
}
