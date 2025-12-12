#pragma once

#include "triglav/Math.hpp"

namespace triglav::editor {

auto create_box_mesh(const Vector3 min, const Vector3 max)
{
   static constexpr std::array<u32, 24> indices{0, 1, 1, 3, 3, 2, 2, 0, 4, 5, 5, 7, 7, 6, 6, 4, 0, 4, 1, 5, 2, 6, 3, 7};

   std::array vertices{
      Vector3{min.x, min.y, max.z}, Vector3{min.x, max.y, max.z}, Vector3{min.x, min.y, min.z}, Vector3{min.x, max.y, min.z},
      Vector3{max.x, min.y, max.z}, Vector3{max.x, max.y, max.z}, Vector3{max.x, min.y, min.z}, Vector3{max.x, max.y, min.z},
   };
   return std::make_pair(vertices, indices);
}

auto create_filled_box_mesh(const Vector3 min, const Vector3 max)
{
   static constexpr std::array<u32, 36> indices{
      2, 3, 1, 0, 2, 1, 6, 7, 3, 2, 6, 3, 4, 5, 7, 6, 4, 7, 0, 1, 5, 4, 0, 5, 0, 4, 6, 2, 0, 6, 5, 1, 3, 7, 5, 3,
   };

   std::array vertices{
      Vector3{min.x, min.y, max.z}, Vector3{min.x, max.y, max.z}, Vector3{min.x, min.y, min.z}, Vector3{min.x, max.y, min.z},
      Vector3{max.x, min.y, max.z}, Vector3{max.x, max.y, max.z}, Vector3{max.x, min.y, min.z}, Vector3{max.x, max.y, min.z},
   };
   return std::make_pair(vertices, indices);
}

template<u32 CRingCount>
auto create_arrow_mesh(const Vector3 origin, const float shaft_radius, const float shaft_height, const float cone_radius,
                       const float cone_height)
{
   static constexpr auto VERTEX_COUNT = 3 + 3 * CRingCount;
   static constexpr auto INDEX_COUNT = 5 * 3 * CRingCount;
   static constexpr auto ORIGIN_IDX = 0;
   static constexpr auto MID_IDX = 1;
   static constexpr auto TOP_IDX = 2;
   static constexpr auto SHAFT_BOTTOM_IDX = 3;
   static constexpr auto SHAFT_TOP_IDX = 3 + CRingCount;
   static constexpr auto CONE_IDX = 3 + 2 * CRingCount;

   std::array<Vector3, VERTEX_COUNT> vertices;
   std::array<u32, INDEX_COUNT> indices;
   vertices[ORIGIN_IDX] = origin;
   vertices[MID_IDX] = origin + Vector3(0, 0, shaft_height);
   vertices[TOP_IDX] = origin + Vector3(0, 0, shaft_height + cone_height);

   for (u32 i = 0; i < CRingCount; ++i) {
      const auto next = (i + 1) % CRingCount;
      const float angle = i * (2 * g_pi / CRingCount);
      vertices[SHAFT_BOTTOM_IDX + i] = origin + Vector3(shaft_radius * std::sin(angle), shaft_radius * std::cos(angle), 0);
      vertices[SHAFT_TOP_IDX + i] = origin + Vector3(shaft_radius * std::sin(angle), shaft_radius * std::cos(angle), shaft_height);
      vertices[CONE_IDX + i] = origin + Vector3(cone_radius * std::sin(angle), cone_radius * std::cos(angle), shaft_height);

      // Shaft back
      indices[3 * i] = SHAFT_BOTTOM_IDX + i;
      indices[3 * i + 1] = ORIGIN_IDX;
      indices[3 * i + 2] = SHAFT_BOTTOM_IDX + next;

      indices[3 * CRingCount + 6 * i] = SHAFT_BOTTOM_IDX + i;
      indices[3 * CRingCount + 6 * i + 1] = SHAFT_BOTTOM_IDX + next;
      indices[3 * CRingCount + 6 * i + 2] = SHAFT_TOP_IDX + i;
      indices[3 * CRingCount + 6 * i + 3] = SHAFT_BOTTOM_IDX + next;
      indices[3 * CRingCount + 6 * i + 4] = SHAFT_TOP_IDX + next;
      indices[3 * CRingCount + 6 * i + 5] = SHAFT_TOP_IDX + i;

      indices[9 * CRingCount + 3 * i] = CONE_IDX + i;
      indices[9 * CRingCount + 3 * i + 1] = MID_IDX;
      indices[9 * CRingCount + 3 * i + 2] = CONE_IDX + next;

      indices[12 * CRingCount + 3 * i] = CONE_IDX + i;
      indices[12 * CRingCount + 3 * i + 1] = CONE_IDX + next;
      indices[12 * CRingCount + 3 * i + 2] = TOP_IDX;
   }

   return std::make_pair(vertices, indices);
}

template<u32 CSegmentCount>
auto create_ring_mesh(const float radius)
{
   std::array<Vector3, CSegmentCount> vertices;
   std::array<u32, CSegmentCount + 1> indices;
   indices[CSegmentCount] = 0;
   for (u32 i = 0; i < CSegmentCount; ++i) {
      const float angle = i * (2 * g_pi / CSegmentCount);
      vertices[i] = Vector3(radius * cos(angle), radius * sin(angle), 0);
      indices[i] = i;
   }
   return std::make_pair(vertices, indices);
}

template<u32 CSegmentCount>
auto create_scaler_mesh(const Vector3 origin, const float shaft_radius, const float shaft_height, const float box_size)
{
   static constexpr auto VERTEX_COUNT = 1 + 2 * CSegmentCount + 8;
   static constexpr auto INDEX_COUNT = 3 * 3 * CSegmentCount + 36;
   static constexpr auto ORIGIN_IDX = 0;
   static constexpr auto SHAFT_BOTTOM_IDX = 1;
   static constexpr auto SHAFT_TOP_IDX = 1 + CSegmentCount;
   static constexpr auto CUBE_IDX = 1 + 2 * CSegmentCount;

   std::array<Vector3, VERTEX_COUNT> vertices;
   std::array<u32, INDEX_COUNT> indices;

   vertices[ORIGIN_IDX] = origin;
   for (u32 i = 0; i < CSegmentCount; ++i) {
      const auto next = (i + 1) % CSegmentCount;
      const float angle = i * (2 * g_pi / CSegmentCount);
      vertices[SHAFT_BOTTOM_IDX + i] = origin + Vector3{shaft_radius * std::sin(angle), shaft_radius * std::cos(angle), 0};
      vertices[SHAFT_TOP_IDX + i] = origin + Vector3{shaft_radius * std::sin(angle), shaft_radius * std::cos(angle), shaft_height};

      indices[3 * i] = SHAFT_BOTTOM_IDX + i;
      indices[3 * i + 1] = ORIGIN_IDX;
      indices[3 * i + 2] = SHAFT_BOTTOM_IDX + next;

      indices[3 * CSegmentCount + 6 * i] = SHAFT_BOTTOM_IDX + i;
      indices[3 * CSegmentCount + 6 * i + 1] = SHAFT_BOTTOM_IDX + next;
      indices[3 * CSegmentCount + 6 * i + 2] = SHAFT_TOP_IDX + i;
      indices[3 * CSegmentCount + 6 * i + 3] = SHAFT_BOTTOM_IDX + next;
      indices[3 * CSegmentCount + 6 * i + 4] = SHAFT_TOP_IDX + next;
      indices[3 * CSegmentCount + 6 * i + 5] = SHAFT_TOP_IDX + i;
   }

   auto [box_vertices, box_indices] = create_filled_box_mesh(origin + Vector3{-box_size, -box_size, -box_size + shaft_height},
                                                             origin + Vector3{box_size, box_size, box_size + shaft_height});

   std::copy(box_vertices.begin(), box_vertices.end(), vertices.begin() + CUBE_IDX);
   std::transform(box_indices.begin(), box_indices.end(), indices.begin() + 3 * 3 * CSegmentCount,
                  [](const u32 index) { return CUBE_IDX + index; });

   return std::make_pair(vertices, indices);
}


}// namespace triglav::editor