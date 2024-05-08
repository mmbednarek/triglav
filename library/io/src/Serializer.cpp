#include "Serializer.h"

#include <array>

namespace triglav::io {

std::array<u8, 32> g_padding;

Serializer::Serializer(IWriter &writer) :
   m_writer(writer)
{
}

Result<void> Serializer::write_float32(const float value)
{
   if (const auto res = this->add_padding(sizeof(value)); not res.has_value()) {
      return res;
   }

   auto bytes = std::bit_cast<std::array<u8, 4>>(value);
   const auto res = m_writer.write(bytes);
   if (not res.has_value()) {
      return std::unexpected{res.error()};
   }

   return {};
}

Result<void> Serializer::add_padding(const u32 alignment)
{
   const auto mod = m_bytesWritten % alignment;
   if (mod != 0) {
      const auto padding = alignment - mod;

      const auto res = m_writer.write(std::span{g_padding.data(), padding});
      if (not res.has_value()) {
         return std::unexpected{res.error()};
      }
   }

   return {};
}

Result<void> Serializer::write_vec3(const glm::vec3 value)
{
   if (const auto res = this->add_padding(4*sizeof(float)); not res.has_value()) {
      return res;
   }

   auto bytes = std::bit_cast<std::array<u8, 12>>(value);
   const auto res = m_writer.write(bytes);
   if (not res.has_value()) {
      return std::unexpected{res.error()};
   }

   return {};
}

Result<void> Serializer::write_vec4(const glm::vec4 value)
{
   if (const auto res = this->add_padding(4*sizeof(float)); not res.has_value()) {
      return res;
   }

   auto bytes = std::bit_cast<std::array<u8, 16>>(value);
   const auto res = m_writer.write(bytes);
   if (not res.has_value()) {
      return std::unexpected{res.error()};
   }

   return {};
}

}

