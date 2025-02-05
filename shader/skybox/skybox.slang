struct VertexIn
{
    [[vk::location(0)]]
    float3 position : POSITION0;
};

struct ViewProperties {
    float4x4 view;
    float4x4 proj;
    float4x4 invView;
    float4x4 invProj;
    float4x4 orient;
    float nearPlane;
    float farPlane;
};

[[vk::binding(0)]]
uniform ConstantBuffer<ViewProperties> viewProps;

struct FragIn
{
    float4 position: SV_Position;

    [[vk::location(0)]]
    float3 worldPos : POSITION0;
};

FragIn vs_main(VertexIn vertIn) {
    FragIn output;
    output.worldPos = vertIn.position;
    output.position = mul(viewProps.proj, mul(viewProps.orient, float4(vertIn.position, 1.0)));
    return output;
}

[[vk::binding(1)]]
uniform Sampler2D skyboxTex;

struct FragOut
{
    [[vk::location(0)]]
    float4 color : SV_Target0;
    [[vk::location(1)]]
    float4 position : SV_Target1;
    [[vk::location(2)]]
    float4 normal : SV_Target1;
};

static const float oneOverTwoPi = 0.1591550;
static const float oneOverPi    = 0.3183099;
static const float pi     = 3.1415927;
static const float halfPi = 1.5707963;

FragOut fs_main(FragIn fragIn) {
    float3 normal = normalize(fragIn.worldPos);
    float yaw = oneOverTwoPi * (pi + atan2(normal.y, normal.x));
    float pitch = oneOverPi * (halfPi + asin(normal.z));
    float2 uv = float2(yaw, pitch);

    FragOut output;
    output.color = skyboxTex.Sample(uv);
    output.position = float4(fragIn.worldPos, 1.0f);
    output.normal = float4(0.0f, 0.0f, 0.0f, 1.0f);

    return output;
}
