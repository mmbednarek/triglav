#version 450

#include "particle.glsl"

layout(std140, binding = 0) readonly buffer ParticleSSBOIn {
    Particle particlesIn[];
};

layout(std140, binding = 1) buffer ParticleSSBOOut {
    Particle particlesOut[];
};

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint index = gl_GlobalInvocationID.x;

    Particle particleIn = particlesIn[index];

    particlesOut[index].position = particleIn.position + particleIn.velocity/* * ubo.deltaTime */;
    particlesOut[index].velocity = particleIn.velocity;

//    if ((particlesOut[index].position.x <= -32.0) || (particlesOut[index].position.x >= -28)) {
//        particlesOut[index].velocity.x = -particlesOut[index].velocity.x;
//    }
//    if ((particlesOut[index].position.y <= -2.0) || (particlesOut[index].position.y >= 2.0)) {
//        particlesOut[index].velocity.y = -particlesOut[index].velocity.y;
//    }

    if (particlesOut[index].position.z > -0.5) {
        particlesOut[index].velocity.z = -0.5 * particlesOut[index].velocity.z;
        particlesOut[index].velocity.x = 0.5 * particlesOut[index].velocity.x;
        particlesOut[index].velocity.y = 0.5 * particlesOut[index].velocity.y;

        if (length(particlesOut[index].velocity) < 0.01) {
            particlesOut[index].position = vec3(-30, 0, -30);
        }

    } else {
        particlesOut[index].velocity.z += 0.0001;
    }

}