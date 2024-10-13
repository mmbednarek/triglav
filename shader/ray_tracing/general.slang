import ray_tracing;

struct VertexData
{
    float3 position;
    float2 texCoord;
    float3 normal;
    float3 tangent;
    float3 bitangent;
};

struct ObjectData
{
    int*        indicies;
    VertexData* verticies;
};

[[vk::binding(0)]]
uniform RaytracingAccelerationStructure topLevelAS : register(t0);

[[vk::binding(3)]]
StructuredBuffer<ObjectData> objects;

struct PushConstants
{
    float3 lightDir;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

static const float3 COLORS[] = {
    float3(0.5, 1.0, 1.0),
    float3(1.0, 0.5, 1.0),
    float3(1.0, 1.0, 0.5),
    float3(1.0, 0.5, 0.5),
    float3(0.5, 1.0, 0.5),
    float3(0.5, 0.5, 1.0),
};

[shader("closesthit")]
void chit_main(inout ray_payload.Color payload, in ray_tracing.Attributes attribs)
{
    let object = objects[InstanceIndex()];

    const int3 indicies = {
        object.indicies[3 * PrimitiveIndex()],
        object.indicies[3 * PrimitiveIndex() + 1],
        object.indicies[3 * PrimitiveIndex() + 2],
    };

    let vecData0 = object.verticies[indicies.x];
    let vecData1 = object.verticies[indicies.y];
    let vecData2 = object.verticies[indicies.z];


    const float3 barycentrics = {
        1.0 - attribs.barycentrics.x - attribs.barycentrics.y,
        attribs.barycentrics.x,
        attribs.barycentrics.y,
    };

    let positionMatrix = transpose(float3x3(
        vecData0.position.xyz,
        vecData1.position.xyz,
        vecData2.position.xyz,
    ));
    let position = mul(positionMatrix, barycentrics);

    let normalMatrix = transpose(float3x3(
        vecData0.normal.xyz,
        vecData1.normal.xyz,
        vecData2.normal.xyz,
    ));
    let normal = normalize(mul(normalMatrix, barycentrics));

    let worldPos = mul(ObjectToWorld3x4(), float4(position, 1));
    let lightDir = -normalize(pc.lightDir);

    RayDesc rayDesc;
    rayDesc.Origin = worldPos + 0.1*lightDir;
    rayDesc.Direction = lightDir;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000.0;

    ray_payload.Shadow shadowPayload;
    shadowPayload.isShadowed = false;

    TraceRay(
        topLevelAS,
        RAY_FLAG_FORCE_OPAQUE,
        0xFF,
        1,
        0,
        1,
        rayDesc,
        shadowPayload
    );

    float lightVal = max(dot(normal, lightDir), 0);
    if (shadowPayload.isShadowed) {
        lightVal = 0;
    }

    let outColor = COLORS[InstanceIndex() % 6];
    payload.hitValue = outColor * (lightVal + 0.1);
}

[shader("miss")]
void miss_main(inout ray_payload.Color payload)
{
    payload.hitValue = float3(0.2, 0.0, 0.1);
}