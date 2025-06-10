module text_processing;

namespace text_processing
{
    public struct UpdateInfo
    {
        public float4 color;
        public float4 crop;
        public float2 position;
        public uint atlasID;

        public uint characterOffset;
        public uint characterCount;
        public uint glyphBufferOffset;

        public uint dstDrawCall;
        public uint dstVertexOffset;
        public uint dstVertexCount;
    };

    public struct DrawCall
    {
        public uint vertexCount;
        public uint instanceCount;
        public uint firstVertex;
        public uint firstInstance;

        public float4 color;
        public float4 crop;
        public float2 position;
        public uint atlasID;
    };

    public struct Vertex
    {
        [[vk::location(0)]]
        public float2 position : POSITION0;
        [[vk::location(1)]]
        public float2 uv : TEXCOORD0;
    };

    public struct GlyphInfo
    {
        public float2 texCoordTopLeft;
        public float2 texCoordBottomRight;
        public float2 size;
        public float2 advance;
        public float2 padding;
    };
}
