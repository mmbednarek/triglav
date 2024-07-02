#version 450

#include "../common/random.glsl"

#include "particle.glsl"

layout(std430, binding = 0) readonly buffer ParticleSSBOIn {
    Particle particlesIn[];
};

layout(std430, binding = 1) buffer ParticleSSBOOut {
    Particle particlesOut[];
};

layout(push_constant) uniform Constants
{
    float deltaTime;
    uint randomValue;
} pc;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint index = gl_GlobalInvocationID.x;

    Particle particleIn = particlesIn[index];

    particlesOut[index].position = particleIn.position + pc.deltaTime * particleIn.velocity;
    particlesOut[index].velocity = particleIn.velocity;
    particlesOut[index].animation = particleIn.animation + 2*pc.deltaTime;
    particlesOut[index].rotation = particleIn.rotation + pc.deltaTime * particleIn.angularVelocity;
    particlesOut[index].angularVelocity = particleIn.angularVelocity;
    particlesOut[index].scale = particleIn.scale;

    while (particlesOut[index].animation > 1) {
        particlesOut[index].animation -= 1;
    }

    if (particlesOut[index].position.z < -40.0) {
        particlesOut[index].velocity.z = 0.0;
        particlesOut[index].position.z = -0.5;
    } else {
        particlesOut[index].velocity.z -= 10 * pc.deltaTime;
    }

}