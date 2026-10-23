#include "Collision.hpp"

namespace triglav::physics {

namespace {

void apply_impulse(DynamicResolution& resolution, const RigidBox& a, const RigidBox& b, const Vector3& normal, const float restitution)
{
   const float inv_sum = a.inv_mass + b.inv_mass;
   if (inv_sum <= 0.0f)
      return;

   const float vn = (resolution.velocity_a - resolution.velocity_b).dot(normal);
   if (vn >= 0.0f)
      return;

   const float j = -(1.0f + restitution) * vn / inv_sum;
   resolution.velocity_a += normal * (j * a.inv_mass);
   resolution.velocity_b -= normal * (j * b.inv_mass);
}

bool resolve_overlap(DynamicResolution& resolution, const RigidBox& a, const RigidBox& b, const float restitution)
{
   const Vector3 d = a.box.centroid() + resolution.movement_a - b.box.centroid() - resolution.movement_b;
   const Vector3 ha = a.box.half_extent();
   const Vector3 hb = b.box.half_extent();

   Vector3 penetration{};
   auto best = Axis::X;
   for (const auto axis : Vector3::AXES) {
      penetration[axis] = ha[axis] + hb[axis] - std::fabs(d[axis]);
      if (penetration[axis] <= 0.0f)
         return false;// separated on this axis
      if (penetration[axis] < penetration[best])
         best = axis;
   }

   const float inv_sum = a.inv_mass + b.inv_mass;
   if (inv_sum <= 0.0f)
      return true;

   const float sign = d[best] < 0.0f ? -1.0f : 1.0f;
   Vector3 normal{};
   normal[best] = sign;
   const Vector3 push = normal * penetration[best];

   resolution.movement_a += push * (a.inv_mass / inv_sum);
   resolution.movement_b += push * (-b.inv_mass / inv_sum);
   apply_impulse(resolution, a, b, normal, restitution);
   return true;
}

}// namespace

CollisionResponse get_slide_response(const LevelBVH& bvh, geometry::BoundingBox bounding_box, const Vector3& movement)
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

      const auto hit = bvh.sweep_cast(bounding_box, remaining);
      out_normal = hit.normal;

      if (hit.entity_id == world::NO_ENTITY || hit.time >= 1.0f) {
         bounding_box.translate(remaining);
         total = total + remaining;
         break;
      }

      const Vector3 advance = remaining * hit.time + hit.normal * SKIN_MARGIN;
      bounding_box.translate(advance);
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

DynamicResolution resolve_aabb_collision(const RigidBox& a, const RigidBox& b, const float dt, const float restitution)
{
   DynamicResolution result{};
   result.velocity_a = a.velocity;
   result.velocity_b = b.velocity;

   result.hit = resolve_overlap(result, a, b, restitution);

   if (!result.hit) {
      // Work in A's frame: B is stationary, A moves with the relative displacement.
      const Vector3 disp = (a.velocity - b.velocity) * dt;
      const Vector3 delta = b.box.centroid() + result.movement_a - a.box.centroid() - result.movement_b;
      const Vector3 sum = a.box.half_extent() + b.box.half_extent();

      float t_entry = -INF;
      float t_exit = INF;
      Vector3 normal;// points from B toward A

      for (const auto axis : Vector3::AXES) {
         const float d = disp[axis];
         const float dl = delta[axis];
         const float s = sum[axis];

         if (d == 0.0f) {
            if (std::fabs(dl) >= s) {
               t_entry = INF;
               break;
            }// parallel and outside the slab
            continue;
         }

         float t1 = (dl - s) / d;
         float t2 = (dl + s) / d;
         if (t1 > t2)
            std::swap(t1, t2);

         if (t1 > t_entry) {
            t_entry = t1;
            // Opposes A's direction of travel on the entry axis.
            normal = {};
            normal[axis] = d > 0.0f ? -1.0f : 1.0f;
         }
         t_exit = std::min(t_exit, t2);
      }

      result.hit = (t_entry <= t_exit) && (t_entry >= 0.0f) && (t_entry <= 1.0f);
      if (result.hit) {
         const float t_impact = std::max(0.0f, t_entry - SKIN_MARGIN);
         result.movement_a += a.velocity * (dt * t_impact);
         result.movement_b += b.velocity * (dt * t_impact);

         apply_impulse(result, a, b, normal, restitution);

         const float remaining = dt * (1.0f - t_impact);
         result.movement_a += a.velocity * remaining;
         result.movement_b += b.velocity * remaining;
         return result;
      }
   }

   // No sweep hit (or overlap was just corrected): plain integration.
   result.movement_a += a.velocity * dt;
   result.movement_b += b.velocity * dt;
   return result;
}

}// namespace triglav::physics
