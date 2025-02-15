#include "Glb.hpp"

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

   GlbChunkHeader jsonHeader{};
   if (!stream.read({reinterpret_cast<u8*>(&jsonHeader), sizeof(GlbChunkHeader)}).has_value()) {
      return std::nullopt;
   }
   if (jsonHeader.chunkType != 0x4E4F534A) {
      return std::nullopt;
   }

   if (stream.seek(io::SeekPosition::Current, jsonHeader.chunkLength) != io::Status::Success) {
      return std::nullopt;
   }

   GlbChunkHeader binaryHeader{};
   if (!stream.read({reinterpret_cast<u8*>(&binaryHeader), sizeof(GlbChunkHeader)}).has_value()) {
      return std::nullopt;
   }
   if (binaryHeader.chunkType != 0x004E4942) {
      return std::nullopt;
   }

   return GlbInfo{
      .jsonSize = jsonHeader.chunkLength,
      .jsonOffset = sizeof(GlbHeader) + sizeof(GlbChunkHeader),
      .binarySize = binaryHeader.chunkLength,
      .binaryOffset = sizeof(GlbHeader) + sizeof(GlbChunkHeader) + jsonHeader.chunkLength + sizeof(GlbChunkHeader),
   };
}

}// namespace triglav::gltf
