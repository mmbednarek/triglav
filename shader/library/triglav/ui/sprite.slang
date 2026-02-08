implementing ui;

namespace triglav::ui {

public struct SpritePrimitive
{
    public float4 spriteRect;
    public float4 uvRect;
    public float4 croppingMask;
    public uint32_t imageID;
    public uint padding[3];
};

public struct SpriteWriteData
{
    public SpritePrimitive primitive;
    public uint dstIndex;
    public uint padding[3];
};

public struct SpriteDrawCall {
    public uint vertexCount;
    public uint instanceCount;
    public uint firstVertex;
    public uint firstInstance;

    public SpritePrimitive primitive;
};

}
