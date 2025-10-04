module rectangle_api;

namespace rectangle_api {

public struct Primitive
{
    public float4 dimensions;
    public float4 borderRadius;
    public float4 borderColor;
    public float4 backgroundColor;
    public float4 croppingMask;
    public float borderWidth;
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