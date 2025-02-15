#include "BufferManager.hpp"

#include <fmt/core.h>

namespace triglav::gltf {

BufferManager::BufferManager(Document& document, io::IFile* binaryChunk, const MemoryOffset binaryChunkOffset) :
    m_document(document)
{
   for (const auto& buffer : m_document.buffers) {
      if (buffer.uri.has_value()) {
         auto fileHandle = io::open_file(io::Path(*buffer.uri), io::FileOpenMode::Read);
         if (!fileHandle.has_value()) {
            throw std::runtime_error(fmt::format("gltf::BufferManager: could not open buffer {}", *buffer.uri));
         }
         m_buffers.emplace_back(BufferWithOffset{fileHandle->get(), 0});
         m_ownedFiles.emplace_back(std::move(*fileHandle));
      } else {
         m_buffers.emplace_back(BufferWithOffset{binaryChunk, binaryChunkOffset});
      }
   }
}

io::Deserializer BufferManager::read_buffer_view(const u32 bufferViewID, const MemoryOffset additionalOffset) const
{
   const auto& bufferView = m_document.bufferViews.at(bufferViewID);

   auto& buffer = m_buffers.at(bufferView.buffer);
   buffer.stream->seek(io::SeekPosition::Begin, buffer.offset + bufferView.byteOffset + additionalOffset);

   return io::Deserializer(*buffer.stream);
}

}// namespace triglav::gltf
