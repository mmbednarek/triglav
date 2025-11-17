#pragma once

#include "Stream.hpp"

#include <string_view>

namespace triglav::io {

class StringReader final : public IReader
{
 public:
   explicit StringReader(std::string_view string_ref);

   [[nodiscard]] Result<MemorySize> read(std::span<u8> buffer) override;

 private:
   std::string_view m_string_ref;
   MemoryOffset m_offset{};
};

}// namespace triglav::io
