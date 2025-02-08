import particle;

[[vk::binding(0)]]
StructuredBuffer<Particle> particlesIn;

[[vk::binding(1)]]
RWStructuredBuffer<Particle> particlesOut;

[[vk::binding(2)]]
cbuffer Constants
{
    float g_deltaTime;
    uint g_randomValue;
};

[numthreads(256, 1, 1)]
void cs_main(uint index: SV_DispatchThreadID)
{
    Particle particleIn = particlesIn[index];

    particlesOut[index].position = particleIn.position + g_deltaTime * particleIn.velocity;
    particlesOut[index].velocity = particleIn.velocity;
    particlesOut[index].animation = particleIn.animation + 2*g_deltaTime;
    particlesOut[index].rotation = particleIn.rotation + g_deltaTime * particleIn.angularVelocity;
    particlesOut[index].angularVelocity = particleIn.angularVelocity;
    particlesOut[index].scale = particleIn.scale;

    while (particlesOut[index].animation > 1) {
        particlesOut[index].animation -= 1;
    }

    if (particlesOut[index].position.z < -40.0) {
        particlesOut[index].velocity.z = 0.0;
        particlesOut[index].position.z = -0.5;
    }
}
