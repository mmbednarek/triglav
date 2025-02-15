#pragma once

#include "Gltf.hpp"

#include "triglav/io/Deserializer.hpp"
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

   explicit BufferManager(Document& document, io::IFile* binaryChunk = nullptr, MemoryOffset binaryChunkOffset = 0);

   [[nodiscard]] io::Deserializer read_buffer_view(u32 bufferViewID, MemoryOffset additionalOffset = 0) const;

 private:
   Document& m_document;
   std::vector<BufferWithOffset> m_buffers;
   std::vector<io::IFileUPtr> m_ownedFiles;
};

}// namespace triglav::gltf