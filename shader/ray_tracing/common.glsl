#ifndef RAY_TRACING_COMMON_H
#define RAY_TRACING_COMMON_H

struct HitPayload {
    vec3 hitValue;
};

struct GlobalUbo
{
    mat4 viewInverse;
    mat4 projInverse;
};

struct VertexData
{
    vec3 position;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
};

#endif // RAY_TRACING_COMMON_H
