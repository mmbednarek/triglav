#include "DebugMeshes.h"

#include <cstdint>
#include <vector>

namespace renderer {

Mesh create_sphere(const int segment_count, const int ring_count, const float radius)
{
   std::vector<uint32_t> indicies{};
   std::vector<Vertex> vertices{};

   vertices.push_back({
           {0, 0, -radius},
           {0.5f, 0},
           {0.0f, 0.0f, -1.0f},
   });

   size_t last_base_index = 0;

   for (int ring = 1; ring < ring_count; ++ring) {
      const auto v          = static_cast<float>(ring) / static_cast<float>(ring_count);
      const auto pitch      = v * M_PI + M_PI / 2.0f;
      const auto cos_pitch  = cos(pitch);
      const auto norm_z     = -sin(pitch);
      const auto z          = radius * norm_z;
      const auto base_index = vertices.size();

      for (int segment = 0; segment <= segment_count; ++segment) {
         const auto u      = 1.0f - static_cast<float>(segment) / static_cast<float>(segment_count);
         const auto yaw    = 2 * M_PI * u;
         const auto norm_x = -cos_pitch * sin(yaw);
         const auto x      = radius * norm_x;
         const auto norm_y = cos_pitch * cos(yaw);
         const auto y      = radius * norm_y;

         vertices.push_back({
                 {x, y, z},
                 {u, v},
                 {norm_x, norm_y, norm_z}
         });
      }

      if (ring == 1) {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;
            indicies.push_back(base_index + segment);
            indicies.push_back(0);
            indicies.push_back(base_index + next_segment);
         }
      } else {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;

            indicies.push_back(base_index + segment);
            indicies.push_back(last_base_index + segment);
            indicies.push_back(last_base_index + next_segment);

            indicies.push_back(base_index + segment);
            indicies.push_back(last_base_index + next_segment);
            indicies.push_back(base_index + next_segment);
         }
      }

      last_base_index = base_index;
   }

   vertices.push_back({
           {0, 0, radius},
           {0.5, 1},
           {0.0f, 0.0f, 1.0f},
   });
   const auto last_index = vertices.size() - 1;

   for (int segment = 0; segment < segment_count; ++segment) {
      const auto next_segment = segment + 1;
      indicies.push_back(last_base_index + segment);
      indicies.push_back(last_base_index + next_segment);
      indicies.push_back(last_index);
   }

   return {std::move(indicies), std::move(vertices)};
}

Mesh create_cilinder(const int segment_count, const int ring_count, const float radius, const float depth)
{
   std::vector<uint32_t> indicies{};
   std::vector<Vertex> vertices{};

   vertices.push_back({
           {0, 0, -depth},
           {0.5, 0},
           {0.0f, 0.0f, -1.0f},
   });

   size_t last_base_index = 0;

   for (int ring = 0; ring <= ring_count; ++ring) {
      const auto v          = static_cast<float>(ring) / static_cast<float>(ring_count);
      const auto z          = 2 * depth * v - depth;
      const auto base_index = vertices.size();

      for (int segment = 0; segment <= segment_count; ++segment) {
         const auto u     = static_cast<float>(segment) / static_cast<float>(segment_count);
         const auto angle = 2 * M_PI * segment / segment_count;
         const auto norm_x = -sin(angle);
         const auto x     = radius * norm_x;
         const auto norm_y = cos(angle);
         const auto y     = radius * norm_y;

         vertices.push_back({
                 {x, y, z},
                 {u, v},
            {norm_x, norm_y, 0.0f},
         });
      }

      if (ring == 0) {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;
            indicies.push_back(base_index + segment);
            indicies.push_back(base_index + next_segment);
            indicies.push_back(0);
         }
      } else {
         for (int segment = 0; segment < segment_count; ++segment) {
            const auto next_segment = segment + 1;

            indicies.push_back(base_index + segment);
            indicies.push_back(last_base_index + next_segment);
            indicies.push_back(last_base_index + segment);

            indicies.push_back(base_index + segment);
            indicies.push_back(base_index + next_segment);
            indicies.push_back(last_base_index + next_segment);
         }
      }

      last_base_index = base_index;
   }

   vertices.push_back({
           {0, 0, depth},
           {0.5, 1},
           {0.0f, 0.0f, 1.0f},
   });
   const auto last_index = vertices.size() - 1;

   for (int segment = 0; segment < segment_count; ++segment) {
      const auto next_segment = segment + 1;
      indicies.push_back(last_base_index + segment);
      indicies.push_back(last_index);
      indicies.push_back(last_base_index + next_segment);
   }

   return {std::move(indicies), std::move(vertices)};
}

}// namespace renderer
