#pragma once

namespace triglav::editor {

auto create_box_mesh()
{
   static constexpr std::array vertices{
      Vector3{0, 0, 1}, Vector3{0, 1, 1}, Vector3{0, 0, 0}, Vector3{0, 1, 0},
      Vector3{1, 0, 1}, Vector3{1, 1, 1}, Vector3{1, 0, 0}, Vector3{1, 1, 0},
   };
   static constexpr std::array<u32, 24> indices{0, 1, 1, 3, 3, 2, 2, 0, 4, 5, 5, 7, 7, 6, 6, 4, 0, 4, 1, 5, 2, 6, 3, 7};
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


}// namespace triglav::editor