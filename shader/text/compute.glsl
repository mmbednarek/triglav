#version 450

#extension GL_EXT_debug_printf : enable

struct GlyphInfo
{
    vec2 texCoordTopLeft;
    vec2 texCoordBottomRight;
    vec2 size;
    vec2 advance;
    vec2 padding;
};

struct GlyphVertex
{
    vec2 position;
    vec2 texCoord;
};

layout(std430, binding = 0) readonly buffer GlyphInfoBuffer
{
    GlyphInfo glyphs[];
};

layout(std430, binding = 1) readonly buffer CharacterBuffer
{
    uint characters[];
};

layout(std430, binding = 2) buffer VertexOutBuffer
{
    GlyphVertex vertices[];
};

uint next_power_of_two(int value)
{
    uint result = 1;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

shared float position[256];

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    const uint index = gl_GlobalInvocationID.x;
    if (index >= characters.length())
        return;

    const uint lenNextPow = next_power_of_two(characters.length());

    if (index == 0) {
        position[index] = 0;
    } else {
        uint prevChar = characters[index - 1];
        position[index] = glyphs[prevChar].advance.x;
    }

    memoryBarrierShared();

    for (uint i = 2; i <= lenNextPow; i = i << 1) {
        const uint iHalf = i >> 1;
        if (index % i >= iHalf) {
            position[index] += position[index - (index % iHalf) - 1];
        }
        memoryBarrierShared();
    }

    GlyphInfo glyph = glyphs[characters[index]];
    const float left = position[index] + glyph.padding.x;
    const float right = position[index] + glyph.padding.x + glyph.size.x;
    const float top = -glyph.padding.y;
    const float bottom = -glyph.padding.y + glyph.size.y;

    // top left
    vertices[6*index] = GlyphVertex(vec2(left, top), glyph.texCoordTopLeft);
    // bottom left
    vertices[6*index + 1] = GlyphVertex(vec2(left, bottom), vec2(glyph.texCoordTopLeft.x, glyph.texCoordBottomRight.y));
    // top right
    vertices[6*index + 2] = GlyphVertex(vec2(right, top), vec2(glyph.texCoordBottomRight.x, glyph.texCoordTopLeft.y));
    // top right
    vertices[6*index + 3] = GlyphVertex(vec2(right, top), vec2(glyph.texCoordBottomRight.x, glyph.texCoordTopLeft.y));
    // bottom left
    vertices[6*index + 4] = GlyphVertex(vec2(left, bottom), vec2(glyph.texCoordTopLeft.x, glyph.texCoordBottomRight.y));
    // bottom right
    vertices[6*index + 5] = GlyphVertex(vec2(right, bottom), glyph.texCoordBottomRight);
}