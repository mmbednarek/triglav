import ray_payload;
import ray_tracing;

[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS : register(t0);

[[vk::binding(3)]]
StructuredBuffer<ray_tracing.ObjectData> objects;

struct PushConstants
{
    float3 lightDir;
};
[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

[shader("closesthit")]
void chit_main(inout ray_payload.AmbientOcclusion payload, in ray_tracing.Attributes attribs)
{
    let object = objects[InstanceIndex()];
    let hitInfo = ray_tracing.ObjectHitInfo(object, PrimitiveIndex(), attribs, ObjectToWorld3x4());

    let lightDir = -normalize(pc.lightDir);

    // ray_payload.Shadow shadowPayload;
    // shadowPayload.isShadowed = false;

    // Trace a shadow
    // RayDesc rayDesc;
    // rayDesc.Origin = hitInfo.worldSpacePosition;
    // rayDesc.Direction = lightDir;
    // rayDesc.TMin = 0.01;
    // rayDesc.TMax = 1000.0;

    // TraceRay(
    //     topLevelAS,
    //     RAY_FLAG_FORCE_OPAQUE,
    //     0xFF,
    //     1,
    //     0,
    //     1,
    //     rayDesc,
    //     shadowPayload
    // );

    payload.distance = length(hitInfo.worldSpacePosition - WorldRayOrigin());
    payload.isOccluded = true;
}

[shader("miss")]
void miss_main(inout ray_payload.AmbientOcclusion payload)
{
    payload.distance = 2.0f;
    payload.isOccluded = false;
}