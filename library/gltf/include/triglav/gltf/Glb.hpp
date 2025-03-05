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
   u32 chunkLength;
   u32 chunkType;
};

struct GlbInfo
{
   MemorySize jsonSize;
   MemorySize jsonOffset;
   MemorySize binarySize;
   MemorySize binaryOffset;
};

std::optional<GlbInfo> read_glb_info(io::ISeekableStream& stream);

struct GlbResource
{
   std::unique_ptr<Document> document;
   io::IFileUPtr glbFileHandle;
   BufferManager bufferManager;
};

std::optional<GlbResource> open_glb_file(const io::Path& path);

}// namespace triglav::gltf
