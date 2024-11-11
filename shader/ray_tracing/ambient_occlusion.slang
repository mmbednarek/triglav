import ray_payload;
import ray_tracing;

[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS : register(t0);

[[vk::binding(4)]]
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

    payload.distance = length(hitInfo.worldSpacePosition - WorldRayOrigin());
}

[shader("miss")]
void miss_main(inout ray_payload.AmbientOcclusion payload)
{
    payload.distance = 4.0f;
}