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
    public float4 viewPos;
    public float4 forward;
    public float4 right;
    public float4 up;
    public float nearPlane;
    public float farPlane;
    public float angle;
    public float aspect;

    public float to_linear_depth(const float depth)
    {
        return (2.0 * this.nearPlane * this.farPlane) / (this.farPlane + this.nearPlane - depth * (this.farPlane - this.nearPlane));
    }

    public float to_world_depth(const float depth)
    {
        return depth * (this.farPlane - this.nearPlane);
    }

    public float3 frag_position(float2 frag)
    {
        const float near_height_half = tan(this.angle / 2.0) * this.nearPlane;
        const float near_width_half = near_height_half * this.aspect;

        float2 coord = 2*frag - float2(1, 1);
        coord.y *= -1;

        return this.viewPos.xyz + this.forward.xyz * this.nearPlane + this.right.xyz * coord.x * near_width_half + this.up.xyz * coord.y * near_height_half;
    }

    public float3 frag_direction(float2 frag)
    {
        const float near_height_half = tan(this.angle / 2.0) * this.nearPlane;
        const float near_width_half = near_height_half * this.aspect;

        float2 coord = 2*frag - float2(1, 1);
        coord.y *= -1;

        return this.right.xyz * coord.x * near_width_half + this.up.xyz * coord.y * near_height_half;
    }

    public float3 depth_world_pos(Texture2D<float> depth_buffer, float2 screen_pos)
    {
        float near_height_half = tan(this.angle / 2.0) * this.nearPlane;
        float near_width_half = near_height_half * this.aspect;

        int3 db_dims;
        depth_buffer.GetDimensions(0, db_dims.x, db_dims.y, db_dims.z);
        int3 tex_coord = int3(int2(screen_pos * float2(db_dims.xy)), 0);

        float depth = this.to_linear_depth(depth_buffer.Load(tex_coord));
        float depth_ratio = depth / this.nearPlane;

        float2 screen_coord = 2 * screen_pos - float2(1);
        screen_coord.y *= -1;

        float4 view_position = float4(
            screen_coord.x * near_width_half * depth_ratio,
            screen_coord.y * near_height_half * depth_ratio,
            -depth,
            1.0
        );

        float4 float_pos = mul(this.invView, view_position);
        return float_pos.xyz;
    }

    public float3 depth_view_pos(Texture2D<float> depth_buffer, float2 screen_pos)
    {
        float near_height_half = tan(this.angle / 2.0) * this.nearPlane;
        float near_width_half = near_height_half * this.aspect;

        int3 db_dims;
        depth_buffer.GetDimensions(0, db_dims.x, db_dims.y, db_dims.z);
        int3 tex_coord = int3(int2(screen_pos * float2(db_dims.xy)), 0);

        float depth = this.to_linear_depth(depth_buffer.Load(tex_coord));
        float depth_ratio = depth / this.nearPlane;

        float2 screen_coord = 2 * screen_pos - float2(1);
        screen_coord.y *= -1;

        return float3(
            screen_coord.x * near_width_half * depth_ratio,
            screen_coord.y * near_height_half * depth_ratio,
            -depth
        );
    }
};

}