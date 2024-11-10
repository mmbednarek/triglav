import ray_tracing;
import ray_payload;

[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS : register(t0);

[[vk::binding(1)]]
[vk::image_format("r16f")]
uniform RWTexture2D<float> outImage : register(t1);

[[vk::binding(2)]]
uniform ConstantBuffer<ray_tracing.ObjectTransform> transform;

[shader("raygeneration")]
void rgen_main()
{
    let pixelCenter = float2(DispatchRaysIndex().xy) + float2(0.5);
    let inUV        = pixelCenter / float2(DispatchRaysDimensions().xy);
    let d           = inUV * 2.0 - 1.0;

    let target    = mul(transform.projInverse, float4(d.x, d.y, 1, 1));

    RayDesc rayDesc;
    rayDesc.Origin    = mul(transform.viewInverse, float4(0, 0, 0, 1)).xyz;
    rayDesc.Direction = mul(transform.viewInverse, float4(normalize(target.xyz), 0)).xyz;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000.0;

    ray_payload.Color colorPayload;

    TraceRay(
        topLevelAS,
        RAY_FLAG_FORCE_OPAQUE,
        0xFF,
        0,
        0,
        0,
        rayDesc,
        colorPayload
    );

    // outImage[DispatchRaysIndex().xy] = float4(float3(colorPayload.ambientOcclusionTerm * (0.2 + 0.8*colorPayload.shadowTerm)), 1.0);
    outImage[DispatchRaysIndex().xy] = colorPayload.ambientOcclusionTerm;
}
