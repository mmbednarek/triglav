#include "DebugMeshes.h"

#include <cstdint>
#include <map>
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
         const auto u      = static_cast<float>(segment) / static_cast<float>(segment_count);
         const auto angle  = 2 * M_PI * segment / segment_count;
         const auto norm_x = -sin(angle);
         const auto x      = radius * norm_x;
         const auto norm_y = cos(angle);
         const auto y      = radius * norm_y;

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

Mesh create_inner_box(const float width, const float height, const float depth)
{
   const auto widthHalf  = width / 2.0f;
   const auto heightHalf = height / 2.0f;
   const auto depthHalf  = depth / 2.0f;

   std::vector<glm::vec3> vertices{
           { widthHalf,  depthHalf, -heightHalf},
           { widthHalf, -depthHalf, -heightHalf},
           { widthHalf,  depthHalf,  heightHalf},
           { widthHalf, -depthHalf,  heightHalf},
           {-widthHalf,  depthHalf, -heightHalf},
           {-widthHalf, -depthHalf, -heightHalf},
           {-widthHalf,  depthHalf,  heightHalf},
           {-widthHalf, -depthHalf,  heightHalf},
   };

   std::vector<glm::vec2> uvs{
           {0.500000, 1.000000},
           {0.500000, 0.666666},
           {0.250000, 0.666666},
           {0.250000, 0.333333},
           {0.000000, 0.333333},
           {1.000000, 0.666666},
           {1.000000, 0.333333},
           {0.750000, 0.333333},
           {0.500000, 0.333333},
           {0.500000, 0.000000},
           {0.250000, 0.000000},
           {0.750000, 0.666666},
           {0.250000, 1.000000},
           {0.000000, 0.666666},
   };

   std::vector<glm::vec3> normals{
           {-0.0000, -1.0000, -0.0000},
           {-0.0000, -0.0000, -1.0000},
           { 1.0000, -0.0000, -0.0000},
           {-0.0000,  1.0000, -0.0000},
           {-1.0000, -0.0000, -0.0000},
           {-0.0000, -0.0000,  1.0000},
   };

   std::vector<object_reader::Face> faces{};
   faces.push_back(object_reader::Face{
           object_reader::Index{5, 1, 1},
           object_reader::Index{1, 2, 1},
           object_reader::Index{3, 3, 1}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{3, 3, 2},
           object_reader::Index{4, 4, 2},
           object_reader::Index{8, 5, 2}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{7, 6, 3},
           object_reader::Index{8, 7, 3},
           object_reader::Index{6, 8, 3}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{2,  9, 4},
           object_reader::Index{6, 10, 4},
           object_reader::Index{8, 11, 4}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{1, 2, 5},
           object_reader::Index{2, 9, 5},
           object_reader::Index{4, 4, 5}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{5, 12, 6},
           object_reader::Index{6,  8, 6},
           object_reader::Index{2,  9, 6}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{5,  1, 1},
           object_reader::Index{3,  3, 1},
           object_reader::Index{7, 13, 1}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{3,  3, 2},
           object_reader::Index{8,  5, 2},
           object_reader::Index{7, 14, 2}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{7,  6, 3},
           object_reader::Index{6,  8, 3},
           object_reader::Index{5, 12, 3}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{2,  9, 4},
           object_reader::Index{8, 11, 4},
           object_reader::Index{4,  4, 4}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{1, 2, 5},
           object_reader::Index{4, 4, 5},
           object_reader::Index{3, 3, 5}
   });
   faces.push_back(object_reader::Face{
           object_reader::Index{5, 12, 6},
           object_reader::Index{2,  9, 6},
           object_reader::Index{1,  2, 6}
   });

   return from_object(
           object_reader::Object(std::move(vertices), std::move(uvs), std::move(normals), std::move(faces)));
}

Mesh from_object(const object_reader::Object &object)
{
   std::vector<Vertex> vertices{};
   std::vector<uint32_t> indices{};

   std::map<object_reader::Index, uint32_t> indexMap{};
   uint32_t topIndex{0};
   for (const auto &face : object.faces) {
      for (const auto &index : face.indices) {
         if (indexMap.contains(index)) {
            indices.emplace_back(indexMap[index]);
            continue;
         }

         auto vertex = object.vertices[index.vertex_id - 1];

         indexMap.emplace(index, topIndex);
         vertices.emplace_back(Vertex{
                 {vertex.x, vertex.z, vertex.y},
                 object.uvs[index.uv_id - 1],
                 object.normals[index.normal_id - 1]
         });
         indices.emplace_back(topIndex);
         ++topIndex;
      }
   }

   return Mesh{std::move(indices), std::move(vertices)};
}

}// namespace renderer
