#include "DebugMesh.h"

#include "InternalMesh.h"

namespace geometry {

Mesh create_box(const Extent3D &extent)
{
   const Extent3D half{0.5f * extent.width, 0.5f * extent.height, 0.5f * extent.depth};

   Mesh mesh;
   mesh.add_group({"box", ""});

   mesh.add_vertex(-half.width, -half.height, half.depth);
   mesh.add_vertex(-half.width, half.height, half.depth);
   mesh.add_vertex(-half.width, -half.height, -half.depth);
   mesh.add_vertex(-half.width, half.height, -half.depth);
   mesh.add_vertex(half.width, -half.height, half.depth);
   mesh.add_vertex(half.width, half.height, half.depth);
   mesh.add_vertex(half.width, -half.height, -half.depth);
   mesh.add_vertex(half.width, half.height, -half.depth);

   const auto bottom = mesh.add_face(2, 3, 1, 0);
   const auto top    = mesh.add_face(6, 7, 3, 2);
   const auto front  = mesh.add_face(4, 5, 7, 6);
   const auto back   = mesh.add_face(0, 1, 5, 4);
   const auto left   = mesh.add_face(0, 4, 6, 2);
   const auto right  = mesh.add_face(5, 1, 3, 7);

   mesh.set_face_group(bottom, 0);
   mesh.set_face_group(top, 0);
   mesh.set_face_group(front, 0);
   mesh.set_face_group(back, 0);
   mesh.set_face_group(left, 0);
   mesh.set_face_group(right, 0);

   constexpr static auto oneThird  = 1.0f / 3.0f;
   constexpr static auto twoThirds = 2.0f / 3.0f;
   mesh.add_uv(1, oneThird);
   mesh.add_uv(1, twoThirds);
   mesh.add_uv(0.75, twoThirds);
   mesh.add_uv(0.75, oneThird);
   mesh.add_uv(0.5, twoThirds);
   mesh.add_uv(0.5, oneThird);
   mesh.add_uv(0.25, twoThirds);
   mesh.add_uv(0.25, oneThird);
   mesh.add_uv(0, twoThirds);
   mesh.add_uv(0, oneThird);
   mesh.add_uv(0.5, 0);
   mesh.add_uv(0.25, 0);
   mesh.add_uv(0.5, 1);
   mesh.add_uv(0.25, 1);

   mesh.set_face_uvs(bottom, 3, 2, 1, 0);
   mesh.set_face_uvs(top, 5, 4, 2, 3);
   mesh.set_face_uvs(front, 7, 6, 4, 5);
   mesh.set_face_uvs(back, 9, 8, 6, 7);
   mesh.set_face_uvs(left, 11, 7, 5, 10);
   mesh.set_face_uvs(right, 6, 13, 12, 4);

   mesh.recalculate_normals();

   return mesh;
}

Mesh create_sphere(const int segment_count, const int ring_count, const float radius)
{
   Mesh mesh;

   mesh.add_vertex(0, 0, -radius);
   mesh.add_uv(0.5f, 0);
   mesh.add_normal(0.0f, 0.0f, -1.0f);

   size_t last_base_index = 0;

   for (int ring = 1; ring < ring_count; ++ring) {
      const auto v          = static_cast<float>(ring) / static_cast<float>(ring_count);
      const auto pitch      = v * M_PI + M_PI / 2.0f;
      const auto cos_pitch  = cos(pitch);
      const auto norm_z     = -sin(pitch);
      const auto z          = radius * norm_z;
      const auto base_index = mesh.vertex_count();

      for (int segment = 0; segment <= segment_count; ++segment) {
         const auto u      = 1.0f - static_cast<float>(segment) / static_cast<float>(segment_count);
         const auto yaw    = 2 * M_PI * u;
         const auto norm_x = -cos_pitch * sin(yaw);
         const auto x      = radius * norm_x;
         const auto norm_y = cos_pitch * cos(yaw);
         const auto y      = radius * norm_y;

         mesh.add_vertex(x, y, z);
         mesh.add_uv(u, v);
         mesh.add_normal(norm_x, norm_y, norm_z);
      }

      if (ring == 1) {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;
            const auto face         = mesh.add_face(0, base_index + next_segment, base_index + segment);
            mesh.set_face_uvs(face, 0, base_index + next_segment, base_index + segment);
            mesh.set_face_normals(face, 0, base_index + next_segment, base_index + segment);
         }
      } else {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;

            const auto face = mesh.add_face(last_base_index + segment, last_base_index + next_segment, base_index + next_segment, base_index + segment);
            mesh.set_face_uvs(face, last_base_index + segment, last_base_index + next_segment, base_index + next_segment, base_index + segment);
            mesh.set_face_normals(face, last_base_index + segment, last_base_index + next_segment, base_index + next_segment, base_index + segment);
         }
      }

      last_base_index = base_index;
   }

   mesh.add_vertex(0, 0, radius);
   mesh.add_uv(0.5, 1);
   mesh.add_normal(0.0f, 0.0f, 1.0f);
   const auto last_index = mesh.vertex_count() - 1;

   for (int segment = 0; segment < segment_count; ++segment) {
      const auto next_segment = segment + 1;
      const auto face = mesh.add_face(last_base_index + next_segment, last_index, last_base_index + segment);
      mesh.set_face_uvs(face, last_base_index + next_segment, last_index, last_base_index + segment);
      mesh.set_face_normals(face, last_base_index + next_segment, last_index, last_base_index + segment);
   }

   return mesh;
}

Mesh create_cilinder(const int segment_count, const int ring_count, const float radius, const float depth)
{
   Mesh mesh;

   mesh.add_vertex(0, 0, -depth);
   mesh.add_uv(0.5, 0);
   mesh.add_normal(0.0f, 0.0f, -1.0f);

   size_t last_base_index = 0;

   for (int ring = 0; ring <= ring_count; ++ring) {
      const auto v          = static_cast<float>(ring) / static_cast<float>(ring_count);
      const auto z          = 2 * depth * v - depth;
      const auto base_index = mesh.vertex_count();

      for (int segment = 0; segment <= segment_count; ++segment) {
         const auto u      = static_cast<float>(segment) / static_cast<float>(segment_count);
         const auto angle  = 2 * M_PI * segment / segment_count;
         const auto norm_x = -sin(angle);
         const auto x      = radius * norm_x;
         const auto norm_y = cos(angle);
         const auto y      = radius * norm_y;

         mesh.add_vertex(x, y, z);
         mesh.add_uv(u, v);
         mesh.add_normal(norm_x, norm_y, 0.0f);
      }

      if (ring == 0) {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;
            const auto face = mesh.add_face(base_index + segment, base_index + next_segment, 0);
            mesh.set_face_uvs(face, base_index + segment, base_index + next_segment, 0);
            mesh.set_face_normals(face, base_index + segment, base_index + next_segment, 0);
         }
      } else {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;

            const auto face = mesh.add_face(base_index + segment, base_index + next_segment, last_base_index + next_segment, last_base_index + segment);
            mesh.set_face_uvs(face, base_index + segment, base_index + next_segment, last_base_index + next_segment, last_base_index + segment);
            mesh.set_face_normals(face, base_index + segment, base_index + next_segment, last_base_index + next_segment, last_base_index + segment);
         }
      }

      last_base_index = base_index;
   }

   mesh.add_vertex(0, 0, depth);
   mesh.add_uv(0.5, 1);
   mesh.add_normal(0.0f, 0.0f, 1.0f);
   const auto last_index = mesh.vertex_count() - 1;

   for (int segment = 0; segment < segment_count; ++segment) {
      const auto next_segment = segment + 1;
      const auto face = mesh.add_face(last_base_index + segment, last_index, last_base_index + next_segment);
      mesh.set_face_uvs(face, last_base_index + segment, last_index, last_base_index + next_segment);
      mesh.set_face_normals(face, last_base_index + segment, last_index, last_base_index + next_segment);
   }

   return mesh;
}

}// namespace geometry