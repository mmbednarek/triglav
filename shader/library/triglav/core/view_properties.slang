implementing core;

namespace triglav
{

public struct ViewProperties
{
    public float4x4 view;
    public float4x4 proj;
    public float4x4 invView;
    public float4x4 invProj;
    public float4x4 orient;
    public float nearPlane;
    public float farPlane;

    public float to_linear_depth(const float depth)
    {
        return (2.0f * this.nearPlane) / (this.farPlane + this.nearPlane - depth * (this.farPlane - this.nearPlane));
    }
};

}