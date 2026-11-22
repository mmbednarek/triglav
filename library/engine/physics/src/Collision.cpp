#include "Collision.hpp"

namespace triglav::physics {

namespace {

void apply_impulse(DynamicResolution& resolution, const float inv_mass_a, const float inv_mass_b, const Vector3& normal,
                   const float restitution)
{
   const float inv_sum = inv_mass_a + inv_mass_b;
   if (inv_sum <= 0.0f)
      return;

   const float vn = (resolution.velocity_a - resolution.velocity_b).dot(normal);
   if (vn >= 0.0f)
      return;

   const float j = -(1.0f + restitution) * vn / inv_sum;
   resolution.velocity_a += normal * (j * inv_mass_a);
   resolution.velocity_b -= normal * (j * inv_mass_b);
}

class TranslationGuard
{
 public:
   explicit TranslationGuard(Collider& collider) :
       m_collider(collider)
   {
   }

   TranslationGuard(const TranslationGuard&) = delete;
   TranslationGuard& operator=(const TranslationGuard&) = delete;

   void move(const Vector3& offset)
   {
      m_collider.translate(offset);
      m_total = m_total + offset;
   }

   ~TranslationGuard()
   {
      m_collider.translate(m_total * -1.0f);
   }

 private:
   Collider& m_collider;
   Vector3 m_total{};
};

}// namespace

CollisionResponse get_slide_response(const LevelBVH& bvh, Collider& collider, const Vector3& movement)
{
   static constexpr int MAX_ITER = 2;

   Vector3 total{};
   Vector3 remaining = movement;
   Vector3 out_normal{};
   std::array<Vector3, MAX_ITER> planes{};
   mem_size plane_count = 0;

   for (int iter = 0; iter < MAX_ITER; ++iter) {
      if (remaining.length_sq() < TOLERANCE)
         break;

      const auto hit = bvh.sweep_cast(collider.to_const_ref(), remaining);
      out_normal = hit.normal;

      if (hit.entity_id == world::NO_ENTITY || hit.time >= 1.0f) {
         collider.translate(remaining);
         total = total + remaining;
         break;
      }

      const Vector3 advance = remaining * hit.time + hit.normal * SKIN_MARGIN;
      collider.translate(advance);
      total = total + advance;

      const Vector3 leftover = remaining * (1.0f - hit.time);
      Vector3 slide = leftover - hit.normal * leftover.dot(hit.normal);

      for (mem_size plane_id = 0; plane_id < plane_count; ++plane_id) {
         if (slide.dot(planes[plane_id]) < 0.0f) {
            Vector3 crease = hit.normal.cross(planes[plane_id]);
            if (crease.length_sq() < TOLERANCE) {
               slide = Vector3{};
               break;
            }
            crease = crease.normalize();
            slide = crease * leftover.dot(crease);

            for (mem_size j = 0; j < plane_count; ++j) {
               if (j != plane_id && slide.dot(planes[j]) < 0.0f) {
                  slide = {};
                  break;
               }
            }
            break;
         }
      }

      planes[plane_count++] = hit.normal;
      remaining = slide;
   }

   return {total, out_normal};
}

DynamicResolution resolve_collision(RigidBody& a, RigidBody& b, const float dt, const float restitution)
{
   static constexpr int MAX_ITER = 2;

   DynamicResolution result{};
   result.velocity_a = a.velocity;
   result.velocity_b = b.velocity;

   TranslationGuard guard_a(a.collider);
   TranslationGuard guard_b(b.collider);

   float remaining_dt = dt;

   for (int iter = 0; iter < MAX_ITER && remaining_dt > 0.0f; ++iter) {
      // Work in B's frame: B is stationary, A moves with the relative displacement.
      const Vector3 relative = (result.velocity_a - result.velocity_b) * remaining_dt;
      if (relative.length_sq() < TOLERANCE)
         break;

      // Re-fetch refs every iteration, since the colliders were translated.
      const ColliderRef ref_a = a.collider.to_const_ref();
      const ColliderRef ref_b = b.collider.to_const_ref();

      const geometry::SweepHit hit = sweep_cast(ref_b, ref_a, relative);
      if (hit.time >= 1.0f)
         break;

      result.hit = true;

      // Advance both bodies to the moment of impact (minus skin margin).
      const float t_impact = std::max(0.0f, hit.time - SKIN_MARGIN);
      const Vector3 step_a = result.velocity_a * (remaining_dt * t_impact);
      const Vector3 step_b = result.velocity_b * (remaining_dt * t_impact);
      guard_a.move(step_a);
      guard_b.move(step_b);
      result.movement_a += step_a;
      result.movement_b += step_b;

      // hit.normal points from B toward A.
      apply_impulse(result, a.inv_mass, b.inv_mass, hit.normal, restitution);

      remaining_dt *= (1.0f - t_impact);
   }

   // Integrate whatever time is left with the post-impulse velocities.
   result.movement_a += result.velocity_a * remaining_dt;
   result.movement_b += result.velocity_b * remaining_dt;
   return result;
}

}// namespace triglav::physics
