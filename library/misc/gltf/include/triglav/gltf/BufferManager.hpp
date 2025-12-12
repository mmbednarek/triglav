#pragma once

#include "Gltf.hpp"

#include "triglav/io/Deserializer.hpp"
#include "triglav/io/DisplacedStream.hpp"
#include "triglav/io/File.hpp"

namespace triglav::gltf {

class BufferManager
{
 public:
   struct BufferWithOffset
   {
      io::ISeekableStream* stream;
      MemoryOffset offset;
   };

   explicit BufferManager(Document& document, io::IFile* binary_chunk = nullptr, MemoryOffset binary_chunk_offset = 0);

   [[nodiscard]] io::Deserializer read_buffer_view(u32 buffer_view_id, MemoryOffset additional_offset = 0) const;
   [[nodiscard]] io::DisplacedStream buffer_view_to_stream(u32 buffer_view_id, MemoryOffset additional_offset = 0) const;

 private:
   Document& m_document;
   std::vector<BufferWithOffset> m_buffers;
   std::vector<io::IFileUPtr> m_owned_files;
};

}// namespace triglav::gltf