import text_processing;

[[vk::binding(0)]]
uniform StructuredBuffer<text_processing::UpdateInfo> g_textUpdates;

[[vk::binding(1)]]
uniform StructuredBuffer<int> g_charBuffer;

[[vk::binding(2)]]
uniform StructuredBuffer<text_processing::GlyphInfo> g_glyphInfos;

[[vk::binding(3)]]
uniform RWStructuredBuffer<text_processing::DrawCall> g_outDrawCalls;

[[vk::binding(4)]]
uniform RWStructuredBuffer<text_processing::Vertex> g_outVertices;

groupshared text_processing::UpdateInfo g_sharedUpdateInfo;
groupshared uint g_charCountNextPow2;
groupshared float g_position[1024];

uint next_power_of_two(uint value)
{
    uint result = 1;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

[numthreads(1024, 1, 1)]
void cs_main(uint3 groupID: SV_GroupID, uint3 threadID : SV_GroupThreadID)
{
    if (threadID.x == 0) {
        g_sharedUpdateInfo = g_textUpdates[groupID.x];
        g_charCountNextPow2 = next_power_of_two(g_sharedUpdateInfo.characterCount);
    }

    AllMemoryBarrierWithGroupSync();

    if (threadID.x >= g_sharedUpdateInfo.characterCount)
        return;

    if (threadID.x == 0) {
        g_position[threadID.x] = 0;
    } else {
        uint prevChar = g_charBuffer[g_sharedUpdateInfo.characterOffset + threadID.x - 1];
        g_position[threadID.x] = g_glyphInfos[g_sharedUpdateInfo.glyphBufferOffset + prevChar].advance.x;
    }

    AllMemoryBarrierWithGroupSync();

    for (uint i = 2; i <= g_charCountNextPow2; i <<= 1) {
        const uint iHalf = i >> 1;
        if ((threadID.x % i) >= iHalf) {
            g_position[threadID.x] += g_position[threadID.x - (threadID.x % iHalf) - 1];
        }
        AllMemoryBarrierWithGroupSync();
    }

    text_processing::GlyphInfo glyph = g_glyphInfos[g_sharedUpdateInfo.glyphBufferOffset + g_charBuffer[g_sharedUpdateInfo.characterOffset + threadID.x]];
    const float left = g_position[threadID.x] + glyph.padding.x;
    const float right = g_position[threadID.x] + glyph.padding.x + glyph.size.x;
    const float top = -glyph.padding.y;
    const float bottom = -glyph.padding.y + glyph.size.y;
    const uint vertexOffset = g_sharedUpdateInfo.dstVertexOffset + 6*threadID.x;

    if (threadID.x == 0) {
        text_processing::DrawCall drawCall;
        drawCall.vertexCount = g_sharedUpdateInfo.dstVertexCount;
        drawCall.firstVertex = g_sharedUpdateInfo.dstVertexOffset;
        drawCall.instanceCount = 1;
        drawCall.firstInstance = 0;
        drawCall.atlasID = g_sharedUpdateInfo.atlasID;
        drawCall.color = g_sharedUpdateInfo.color;
        drawCall.position = g_sharedUpdateInfo.position;

        g_outDrawCalls[g_sharedUpdateInfo.dstDrawCall] = drawCall;
    }

    // top left
    g_outVertices[vertexOffset] = text_processing::Vertex(float2(left, top), glyph.texCoordTopLeft);
    // bottom left
    g_outVertices[vertexOffset + 1] = text_processing::Vertex(float2(left, bottom), float2(glyph.texCoordTopLeft.x, glyph.texCoordBottomRight.y));
    // top right
    g_outVertices[vertexOffset + 2] = text_processing::Vertex(float2(right, top), float2(glyph.texCoordBottomRight.x, glyph.texCoordTopLeft.y));
    // top right
    g_outVertices[vertexOffset + 3] = text_processing::Vertex(float2(right, top), float2(glyph.texCoordBottomRight.x, glyph.texCoordTopLeft.y));
    // bottom left
    g_outVertices[vertexOffset + 4] = text_processing::Vertex(float2(left, bottom), float2(glyph.texCoordTopLeft.x, glyph.texCoordBottomRight.y));
    // bottom right
    g_outVertices[vertexOffset + 5] = text_processing::Vertex(float2(right, bottom), glyph.texCoordBottomRight);
}