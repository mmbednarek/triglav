#pragma once

#include <array>
#include <istream>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace object_reader {

struct Index
{
   uint32_t vertex_id;
   uint32_t uv_id;
   uint32_t normal_id;

   auto operator<=>(const Index& rhs) const = default;
};

struct Face
{
   std::array<Index, 3> indices;
};

struct Object
{
   std::vector<glm::vec3> vertices;
   std::vector<glm::vec2> uvs;
   std::vector<glm::vec3> normals;
   std::vector<Face> faces;
};

Object read_object(std::istream& stream);

}