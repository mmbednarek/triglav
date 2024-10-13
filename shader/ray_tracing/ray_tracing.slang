module ray_tracing;

namespace ray_tracing
{

public typedef BuiltInTriangleIntersectionAttributes Attributes;


public struct ObjectTransform
{
    public float4x4 viewInverse;
    public float4x4 projInverse;
};

}

__include "ray_payload";