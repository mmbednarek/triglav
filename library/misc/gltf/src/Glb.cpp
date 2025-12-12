#include "Glb.hpp"

#include "triglav/io/LimitedReader.hpp"

#include <cassert>

namespace triglav::gltf {

std::optional<GlbInfo> read_glb_info(io::ISeekableStream& stream)
{
   GlbHeader header{};
   if (!stream.read({reinterpret_cast<u8*>(&header), sizeof(GlbHeader)}).has_value()) {
      return std::nullopt;
   }
   if (header.magic != 0x46546C67) {
      return std::nullopt;
   }

   GlbChunkHeader json_header{};
   if (!stream.read({reinterpret_cast<u8*>(&json_header), sizeof(GlbChunkHeader)}).has_value()) {
      return std::nullopt;
   }
   if (json_header.chunk_type != 0x4E4F534A) {
      return std::nullopt;
   }

   if (stream.seek(io::SeekPosition::Current, json_header.chunk_length) != io::Status::Success) {
      return std::nullopt;
   }

   GlbChunkHeader binary_header{};
   if (!stream.read({reinterpret_cast<u8*>(&binary_header), sizeof(GlbChunkHeader)}).has_value()) {
      return std::nullopt;
   }
   if (binary_header.chunk_type != 0x004E4942) {
      return std::nullopt;
   }

   return GlbInfo{
      .json_size = json_header.chunk_length,
      .json_offset = sizeof(GlbHeader) + sizeof(GlbChunkHeader),
      .binary_size = binary_header.chunk_length,
      .binary_offset = sizeof(GlbHeader) + sizeof(GlbChunkHeader) + json_header.chunk_length + sizeof(GlbChunkHeader),
   };
}

std::optional<GlbResource> open_glb_file(const io::Path& path)
{
   auto file_handle_res = io::open_file(path, io::FileMode::Read);
   if (!file_handle_res.has_value()) {
      return std::nullopt;
   }
   auto& file_handle = **file_handle_res;

   auto glb_info = read_glb_info(file_handle);
   if (!glb_info.has_value()) {
      return std::nullopt;
   }

   file_handle.seek(io::SeekPosition::Begin, static_cast<MemoryOffset>(glb_info->json_offset));

   io::LimitedReader json_reader(file_handle, glb_info->json_size);

   auto doc = std::make_unique<Document>();
   doc->deserialize(json_reader);

   BufferManager manager(*doc, &file_handle, static_cast<MemoryOffset>(glb_info->binary_offset));

   return GlbResource{.document{std::move(doc)}, .glb_file_handle{std::move(*file_handle_res)}, .buffer_manager{std::move(manager)}};
}

}// namespace triglav::gltf
