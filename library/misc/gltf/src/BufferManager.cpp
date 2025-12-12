#include "BufferManager.hpp"

#include <format>
#include <stdexcept>

namespace triglav::gltf {

BufferManager::BufferManager(Document& document, io::IFile* binary_chunk, const MemoryOffset binary_chunk_offset) :
    m_document(document)
{
   for (const auto& buffer : m_document.buffers) {
      if (buffer.uri.has_value()) {
         auto file_handle = io::open_file(io::Path(*buffer.uri), io::FileMode::Read);
         if (!file_handle.has_value()) {
            throw std::runtime_error(std::format("gltf::BufferManager: could not open buffer {}", *buffer.uri));
         }
         m_buffers.emplace_back(BufferWithOffset{file_handle->get(), 0});
         m_owned_files.emplace_back(std::move(*file_handle));
      } else {
         m_buffers.emplace_back(BufferWithOffset{binary_chunk, binary_chunk_offset});
      }
   }
}

io::Deserializer BufferManager::read_buffer_view(const u32 buffer_view_id, const MemoryOffset additional_offset) const
{
   const auto& buffer_view = m_document.buffer_views.at(buffer_view_id);

   auto& buffer = m_buffers.at(buffer_view.buffer);
   buffer.stream->seek(io::SeekPosition::Begin, buffer.offset + buffer_view.byte_offset + additional_offset);

   return io::Deserializer(*buffer.stream);
}

io::DisplacedStream BufferManager::buffer_view_to_stream(const u32 buffer_view_id, const MemoryOffset additional_offset) const
{
   const auto& buffer_view = m_document.buffer_views.at(buffer_view_id);
   auto& buffer = m_buffers.at(buffer_view.buffer);
   const auto offset = buffer.offset + buffer_view.byte_offset + additional_offset;

   buffer.stream->seek(io::SeekPosition::Begin, offset);
   return {*buffer.stream, offset, buffer_view.byte_length};
}

}// namespace triglav::gltf
