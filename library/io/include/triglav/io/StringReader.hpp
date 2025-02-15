#pragma once

#include "Stream.hpp"

#include <string_view>

namespace triglav::io {

class StringReader final : public IReader
{
 public:
   explicit StringReader(std::string_view stringRef);

   [[nodiscard]] Result<MemorySize> read(std::span<u8> buffer) override;

 private:
   std::string_view m_stringRef;
   MemoryOffset m_offset{};
};

}// namespace triglav::io
