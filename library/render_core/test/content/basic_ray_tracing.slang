[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS : register(t0);

[[vk::binding(1)]]
[vk::image_format("rgba8")]
uniform RWTexture2D outImage : register(t1);

struct ViewProperties
{
    float4x4 projInverse;
    float4x4 viewInverse;
};

[[vk::binding(2)]]
uniform ConstantBuffer<ViewProperties> viewProperties;

struct Payload
{
    float4 color;
};

[shader("raygeneration")]
void rgen_main()
{
    let pixelCenter = float2(DispatchRaysIndex().xy) + float2(0.5);
    let inUV        = pixelCenter / float2(DispatchRaysDimensions().xy);
    let d           = inUV * 2.0 - 1.0;

    let target    = mul(viewProperties.projInverse, float4(d.x, d.y, 1, 1));

    RayDesc rayDesc;
    rayDesc.Origin    = mul(viewProperties.viewInverse, float4(0, 0, 0, 1)).xyz;
    rayDesc.Direction = mul(viewProperties.viewInverse, float4(normalize(target.xyz), 0)).xyz;
    rayDesc.TMin      = 0.01;
    rayDesc.TMax      = 1000.0;

    Payload payload;

    TraceRay(
        topLevelAS,
        RAY_FLAG_FORCE_OPAQUE,
        0xFF,
        0,
        0,
        0,
        rayDesc,
        payload
    );

    outImage[DispatchRaysIndex().xy] = payload.color;
}

[shader("miss")]
void rmiss_main(inout Payload payload)
{
    payload.color = float4(0.5, 0.5, 1.0, 1);
}

[shader("closesthit")]
void rchit_main(inout Payload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float4(attribs.barycentrics, 0.0, 1);
}
