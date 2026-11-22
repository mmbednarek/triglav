#pragma once

#include "triglav/Math.hpp"

#include "PhysicsSystem.hpp"

namespace triglav::physics {

struct CollisionResponse
{
   Vector3 movement;
   Vector3 normal;
};

struct RigidBody
{
   Collider collider;
   Vector3 velocity{};
   float inv_mass = 1.0f;
};

struct DynamicResolution
{
   bool hit{};
   Vector3 movement_a;
   Vector3 velocity_a;
   Vector3 movement_b;
   Vector3 velocity_b;
};

constexpr float SKIN_MARGIN = 1e-4f;
constexpr float TOLERANCE = 1e-8f;

CollisionResponse get_slide_response(const LevelBVH& bvh, Collider& collider, const Vector3& movement);
DynamicResolution resolve_collision(RigidBody& a, RigidBody& b, float dt, float restitution = 0.0f);

}// namespace triglav::physics