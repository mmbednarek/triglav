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

#endif // RAY_TRACING_COMMON_H
