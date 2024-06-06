#version 450

#include "particle.glsl"

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out float fragAnimation;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(std140, binding = 1) readonly buffer Particles {
    Particle particlesIn[];
};

vec2 g_positions[4] = {
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, 1.0),
};

void main() {
    const vec2 point = g_positions[gl_VertexIndex];
    const Particle particle = particlesIn[gl_InstanceIndex];

    float sinA = sin(particle.rotation);
    float cosA = cos(particle.rotation);

    mat3 mat = mat3(cosA, sinA, 0, -sinA, cosA, 0, 0, 0, 1);

    vec4 particleViewPos = ubo.view * vec4(particle.position, 1.0);
    particleViewPos.xy += (mat * (particle.scale * vec3(point, 1.0))).xy;

    int frameID = int(16.0  * particle.animation);
    ivec2 framePos = ivec2(frameID % 4, frameID / 4);

    fragAnimation = particle.animation;
    fragTexCoord =  vec2(0.25) * vec2(framePos) + 0.25 * (vec2(0.5) + 0.5*point);
    gl_Position = ubo.proj * particleViewPos;
}
