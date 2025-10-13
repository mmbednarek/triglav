#include "DebugMesh.hpp"

#include "InternalMesh.hpp"

#include <cmath>

namespace triglav::geometry {

using namespace name_literals;

Mesh create_box(const Extent3D& extent)
{
   const Extent3D half{0.5f * extent.width, 0.5f * extent.height, 0.5f * extent.depth};

   Mesh mesh;
   mesh.add_group({"box", "stone.mat"_rc});

   mesh.add_vertex(-half.width, -half.height, half.depth);
   mesh.add_vertex(-half.width, half.height, half.depth);
   mesh.add_vertex(-half.width, -half.height, -half.depth);
   mesh.add_vertex(-half.width, half.height, -half.depth);
   mesh.add_vertex(half.width, -half.height, half.depth);
   mesh.add_vertex(half.width, half.height, half.depth);
   mesh.add_vertex(half.width, -half.height, -half.depth);
   mesh.add_vertex(half.width, half.height, -half.depth);

   const auto bottom = mesh.add_face(2, 3, 1, 0);
   const auto top = mesh.add_face(6, 7, 3, 2);
   const auto front = mesh.add_face(4, 5, 7, 6);
   const auto back = mesh.add_face(0, 1, 5, 4);
   const auto left = mesh.add_face(0, 4, 6, 2);
   const auto right = mesh.add_face(5, 1, 3, 7);

   mesh.set_face_group(bottom, 0);
   mesh.set_face_group(top, 0);
   mesh.set_face_group(front, 0);
   mesh.set_face_group(back, 0);
   mesh.set_face_group(left, 0);
   mesh.set_face_group(right, 0);

   constexpr static auto oneThird = 1.0f / 3.0f;
   constexpr static auto twoThirds = 2.0f / 3.0f;

   std::array uvs{
      glm::vec2{1, oneThird},    glm::vec2{1, twoThirds},  glm::vec2{0.75, twoThirds}, glm::vec2{0.75, oneThird},
      glm::vec2{0.5, twoThirds}, glm::vec2{0.5, oneThird}, glm::vec2{0.25, twoThirds}, glm::vec2{0.25, oneThird},
      glm::vec2{0, twoThirds},   glm::vec2{0, oneThird},   glm::vec2{0.5, 0},          glm::vec2{0.25, 0},
      glm::vec2{0.5, 1},         glm::vec2{0.25, 1},
   };

   mesh.set_face_uvs(bottom, uvs[3], uvs[2], uvs[1], uvs[0]);
   mesh.set_face_uvs(top, uvs[5], uvs[4], uvs[2], uvs[3]);
   mesh.set_face_uvs(front, uvs[7], uvs[6], uvs[4], uvs[5]);
   mesh.set_face_uvs(back, uvs[9], uvs[8], uvs[6], uvs[7]);
   mesh.set_face_uvs(left, uvs[11], uvs[7], uvs[5], uvs[10]);
   mesh.set_face_uvs(right, uvs[6], uvs[13], uvs[12], uvs[4]);

   mesh.recalculate_normals();

   return mesh;
}

Mesh create_sphere(const int segment_count, const int ring_count, const float radius)
{
   Mesh mesh;
   mesh.add_group({"sphere", "stone.mat"_rc});

   std::vector<glm::vec2> uvs{};
   std::vector<glm::vec3> normals{};

   mesh.add_vertex(0, 0, -radius);
   uvs.emplace_back(0.5f, 0);
   normals.emplace_back(0.0f, 0.0f, -1.0f);

   size_t last_base_index = 0;

   for (int ring = 1; ring < ring_count; ++ring) {
      const auto v = static_cast<float>(ring) / static_cast<float>(ring_count);
      const auto pitch = v * static_cast<float>(g_pi) + static_cast<float>(g_pi) / 2.0f;
      const auto cos_pitch = cos(pitch);
      const auto norm_z = -sin(pitch);
      const auto z = radius * norm_z;
      const auto base_index = mesh.vertex_count();

      for (int segment = 0; segment <= segment_count; ++segment) {
         const auto u = 1.0f - static_cast<float>(segment) / static_cast<float>(segment_count);
         const auto yaw = 2.0f * static_cast<float>(g_pi) * u;
         const auto norm_x = -cos_pitch * sin(yaw);
         const auto x = radius * norm_x;
         const auto norm_y = cos_pitch * cos(yaw);
         const auto y = radius * norm_y;

         mesh.add_vertex(x, y, z);
         uvs.emplace_back(u, v);
         normals.emplace_back(norm_x, norm_y, norm_z);
      }

      if (ring == 1) {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;
            const auto face = mesh.add_face(0, base_index + next_segment, base_index + segment);
            mesh.set_face_uvs(face, uvs[0], uvs[base_index + next_segment], uvs[base_index + segment]);
            mesh.set_face_normals(face, normals[0], normals[base_index + next_segment], normals[base_index + segment]);
            mesh.set_face_group(face, 0);
         }
      } else {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;

            const auto face =
               mesh.add_face(last_base_index + segment, last_base_index + next_segment, base_index + next_segment, base_index + segment);
            mesh.set_face_uvs(face, uvs[last_base_index + segment], uvs[last_base_index + next_segment], uvs[base_index + next_segment],
                              uvs[base_index + segment]);
            mesh.set_face_normals(face, normals[last_base_index + segment], normals[last_base_index + next_segment],
                                  normals[base_index + next_segment], normals[base_index + segment]);
            mesh.set_face_group(face, 0);
         }
      }

      last_base_index = base_index;
   }

   mesh.add_vertex(0, 0, radius);
   uvs.emplace_back(0.5, 1);
   normals.emplace_back(0.0f, 0.0f, 1.0f);
   const auto last_index = mesh.vertex_count() - 1;

   for (int segment = 0; segment < segment_count; ++segment) {
      const auto next_segment = segment + 1;
      const auto face = mesh.add_face(last_base_index + next_segment, last_index, last_base_index + segment);
      mesh.set_face_uvs(face, uvs[last_base_index + next_segment], uvs[last_index], uvs[last_base_index + segment]);
      mesh.set_face_normals(face, normals[last_base_index + next_segment], normals[last_index], normals[last_base_index + segment]);
      mesh.set_face_group(face, 0);
   }

   return mesh;
}

/*
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
            const auto face         = mesh.add_face(base_index + segment, base_index + next_segment, 0);
            mesh.set_face_uvs(face, base_index + segment, base_index + next_segment, 0);
            mesh.set_face_normals(face, base_index + segment, base_index + next_segment, 0);
         }
      } else {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;

            const auto face = mesh.add_face(base_index + segment, base_index + next_segment,
                                            last_base_index + next_segment, last_base_index + segment);
            mesh.set_face_uvs(face, base_index + segment, base_index + next_segment,
                              last_base_index + next_segment, last_base_index + segment);
            mesh.set_face_normals(face, base_index + segment, base_index + next_segment,
                                  last_base_index + next_segment, last_base_index + segment);
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
*/

}// namespace triglav::geometry