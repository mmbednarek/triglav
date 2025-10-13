module sprite_api;

namespace sprite_api {

public struct Primitive
{
    public float4 spriteRect;
    public float4 uvRect;
    public float4 croppingMask;
    public uint32_t imageID;
    public uint padding[3];
};

public struct WriteData
{
    public Primitive primitive;
    public uint dstIndex;
    public uint padding[3];
};

public struct DrawCall {
    public uint vertexCount;
    public uint instanceCount;
    public uint firstVertex;
    public uint firstInstance;

    public Primitive primitive;
};

}
