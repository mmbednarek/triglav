#pragma once

#include <cstdint>
#include <string_view>

#include "Crc.hpp"

namespace renderer {

using Name = std::uint64_t;

enum class NameType : uint8_t
{
   Texture        = 0b000,
   Mesh           = 0b001,
   FragmentShader = 0b010,
   VertexShader   = 0b011,
};

constexpr Name make_name(const std::string_view value)
{
   NameType type{};
   if (value.substr(0, 3) == "tex")
      type = NameType::Texture;
   else if (value.substr(0, 3) == "msh")
      type = NameType::Mesh;
   else if (value.substr(0, 3) == "fsh")
      type = NameType::FragmentShader;
   else if (value.substr(0, 3) == "vsh")
      type = NameType::VertexShader;

   return crc::hash_string(value.substr(4)) << 3 | static_cast<std::underlying_type_t<NameType>>(type);
}

constexpr NameType get_name_type(const Name name)
{
   return static_cast<NameType>(name & 0b111);
}

constexpr Name operator""_name(const char *value, const std::size_t count)
{
   return make_name(std::string_view(value, count));
}

}// namespace renderer
