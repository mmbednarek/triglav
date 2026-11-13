#include "SweepCallbacks.hpp"
#include "Geometry.hpp"
#include "Terrain.hpp"

namespace triglav::geometry {

namespace {

using SweepStaticCallback = SweepHit (*)(const TraversableHeader* static_object, const TraversableHeader* dynamic_object, const Vector3& movement_vec);
using SweepDynamicCallback = SweepHit (*)(const TraversableHeader* lhs, const Vector3& lhs_movement, const TraversableHeader* rhs, const Vector3& rhs_movement);

constexpr mem_size CALLBACK_COUNT = static_cast<mem_size>(TraversableKind::MaxValue) * static_cast<mem_size>(TraversableKind::MaxValue);

constexpr mem_size pair_index(const TraversableKind a, const TraversableKind b) {
    return static_cast<mem_size>(a)  + static_cast<mem_size>(b) * static_cast<mem_size>(TraversableKind::MaxValue);
}

constexpr std::array<SweepStaticCallback, CALLBACK_COUNT> generate_static_callbacks();
constexpr std::array<SweepDynamicCallback, CALLBACK_COUNT> generate_dynamic_callbacks();

std::array<SweepStaticCallback, CALLBACK_COUNT> SWEEP_STATIC_CALLBACKS = generate_static_callbacks();
std::array<SweepDynamicCallback, CALLBACK_COUNT> SWEEP_DYNAMIC_CALLBACKS = generate_dynamic_callbacks();

struct TraversableTerrain : TraversableHeader
{
    Terrain* terrain;
};

SweepHit sweep_cast_static_aabb_aabb(const TraversableHeader* static_object, const TraversableHeader* dynamic_object, const Vector3& movement_vec)
{
    auto res = static_object->bouding_box.intersect_swept(dynamic_object->bouding_box, movement_vec);
    return {res.entry_time, res.normal};
}

SweepHit sweep_cast_static_terrain_aabb(const TraversableHeader* static_object, const TraversableHeader* dynamic_object, const Vector3& movement_vec)
{
    return static_cast<const TraversableTerrain*>(static_object)->terrain->generic_sweep_cast(dynamic_object->bouding_box, movement_vec);
}


constexpr std::array<SweepStaticCallback, CALLBACK_COUNT> generate_static_callbacks()
{
    std::array<SweepStaticCallback, CALLBACK_COUNT> result{};
    result[pair_index(TraversableKind::AABB, TraversableKind::AABB)] = sweep_cast_static_aabb_aabb;
    result[pair_index(TraversableKind::Terrain, TraversableKind::AABB)] = sweep_cast_static_terrain_aabb;
    return result;
}

constexpr std::array<SweepDynamicCallback, CALLBACK_COUNT> generate_dynamic_callbacks()
{
    std::array<SweepDynamicCallback, CALLBACK_COUNT> result{};
    return result;
}

}

SweepHit static_sweep_cast(const TraversableHeader* static_object, const TraversableHeader* dynamic_object, const Vector3& movement_vec)
{
    const auto cb = SWEEP_STATIC_CALLBACKS[pair_index(static_object->kind, dynamic_object->kind)];
    if (cb == nullptr) {
        return SweepHit::miss();
    }
    return cb(static_object, dynamic_object, movement_vec);
}

SweepHit dynamic_sweep_cast(const TraversableHeader* lhs, const Vector3& lhs_movement, const TraversableHeader* rhs, const Vector3& rhs_movement)
{
    const auto cb = SWEEP_DYNAMIC_CALLBACKS[pair_index(lhs->kind, lhs->kind)];
    if (cb == nullptr) {
        return SweepHit::miss();
    }
    return cb(lhs, lhs_movement, rhs, rhs_movement);
}

}