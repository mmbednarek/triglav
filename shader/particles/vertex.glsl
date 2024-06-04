#version 450

#include "particle.glsl"

layout(location = 0) out vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(std140, binding = 1) readonly buffer Particles {
    Particle particlesIn[];
};

vec2 g_positions[4] = {
    vec2(-0.5, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, -0.5),
    vec2(0.5, 0.5),
};

void main() {
    const vec2 point = g_positions[gl_VertexIndex];
    const Particle particle = particlesIn[gl_InstanceIndex];

    vec4 particleViewPos = ubo.view * vec4(particle.position, 1.0);
    particleViewPos.xy += point;

    fragTexCoord = vec2(0.5) + point;

    gl_Position = ubo.proj * particleViewPos;
}
