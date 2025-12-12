#pragma once

#include "BufferManager.hpp"
#include "Gltf.hpp"

#include "triglav/Int.hpp"
#include "triglav/io/Stream.hpp"

#include <memory>
#include <optional>

namespace triglav::gltf {

struct GlbHeader
{
   u32 magic;
   u32 version;
   u32 length;
};

struct GlbChunkHeader
{
   u32 chunk_length;
   u32 chunk_type;
};

struct GlbInfo
{
   MemorySize json_size;
   MemorySize json_offset;
   MemorySize binary_size;
   MemorySize binary_offset;
};

std::optional<GlbInfo> read_glb_info(io::ISeekableStream& stream);

struct GlbResource
{
   std::unique_ptr<Document> document;
   io::IFileUPtr glb_file_handle;
   BufferManager buffer_manager;
};

std::optional<GlbResource> open_glb_file(const io::Path& path);

}// namespace triglav::gltf
