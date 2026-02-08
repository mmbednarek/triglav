implementing ui;

namespace triglav::ui {

public struct RectPrimitive
{
    public float4 dimensions;
    public float4 borderRadius;
    public float4 borderColor;
    public float4 backgroundColor;
    public float4 croppingMask;
    public float borderWidth;
    public uint padding[3];
};

public struct RectWriteData
{
    public RectPrimitive primitive;
    public uint dstIndex;
    public uint padding[3];
};

public struct RectDrawCall {
    public uint vertexCount;
    public uint instanceCount;
    public uint firstVertex;
    public uint firstInstance;

    public RectPrimitive primitive;
};

}