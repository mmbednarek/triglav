#ifndef PARTICLE_H
#define PARTICLE_H

struct Particle {
    vec3 position;
    vec3 velocity;
    float animation;
    float rotation;
    float angularVelocity;
    float scale;
};

#endif // PARTICLE_H
