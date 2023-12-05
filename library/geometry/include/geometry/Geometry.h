#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

namespace geometry {

using Index = uint32_t;

constexpr Index g_invalidIndex = std::numeric_limits<Index>::max();

constexpr bool is_valid(const Index index)
{
   return index != g_invalidIndex;
}

struct Vertex
{
   glm::vec3 location;
   glm::vec2 uv;
   glm::vec3 normal;
};

struct IndexedVertex
{
   Index location;
   Index uv;
   Index normal;

   auto operator<=>(const IndexedVertex &rhs) const = default;
};

}// namespace geometry