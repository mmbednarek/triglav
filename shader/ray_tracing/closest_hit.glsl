#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

#include "common.glsl"

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT HitPayload prd;
layout(location = 1) rayPayloadEXT HitPayload refl;

layout(buffer_reference, scalar, std430) readonly buffer IndexBuffer
{
    int indicies[];
};

layout(buffer_reference, scalar, std430) readonly buffer VertexBuffer
{
    VertexData vertices[];
};

struct ObjectData
{
    uint64_t indexBuffer;
    uint64_t vertexBuffer;
};

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 3, std430) readonly buffer ObjectDataBuffer
{
    ObjectData objects[];
};

layout(push_constant) uniform Constants
{
    vec3 viewPosition;
} pc;

void main()
{
    ObjectData object = objects[gl_InstanceCustomIndexEXT];
    IndexBuffer indexBuffer = IndexBuffer(object.indexBuffer);
    VertexBuffer vertexBuffer = VertexBuffer(object.vertexBuffer);

    VertexData vecData0 = vertexBuffer.vertices[indexBuffer.indicies[3 * gl_PrimitiveID]];
    VertexData vecData1 = vertexBuffer.vertices[indexBuffer.indicies[3 * gl_PrimitiveID + 1]];
    VertexData vecData2 = vertexBuffer.vertices[indexBuffer.indicies[3 * gl_PrimitiveID + 2]];

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    const vec3 position = barycentrics.x * vecData0.position + barycentrics.y * vecData1.position + barycentrics.z * vecData2.position;
    const vec3 normal = normalize(barycentrics.x * vecData0.normal + barycentrics.y * vecData1.normal + barycentrics.z * vecData2.normal);
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));

    const vec3 lightDir = normalize(vec3(1, 0, 1));
    const float dotNL = dot(normal, lightDir);
    const float lightVal = max(dotNL, 0);
    // const vec3 viewDir = normalize(worldPos - pc.viewPosition);
    vec3 viewDir = gl_WorldRayDirectionEXT;

     vec3 traceDir = reflect(viewDir, -normal);

    vec3 outColor = vec3(0.5, 1.0, 0.5);

    if (gl_InstanceCustomIndexEXT == 1 && dot(-normal, traceDir) >= 0) {
        const uint  rayFlags = gl_RayFlagsOpaqueEXT;
        const float tMin     = 0.01;
        const float tMax     = 1000.0;

        traceRayEXT(
        topLevelAS, // acceleration structure
        rayFlags, // rayFlags
        0xFF, // cullMask
        0, // sbtRecordOffset
        0, // sbtRecordStride
        0, // missIndex
        worldPos + 0.1*traceDir, // ray origin
        tMin, // ray min range
        traceDir, // ray direction
        tMax, // ray max range
        1// payload (location = 0)
        );

        outColor = refl.hitValue;
    }

    prd.hitValue = outColor * (lightVal + 0.1);
    refl.hitValue = prd.hitValue;
}
