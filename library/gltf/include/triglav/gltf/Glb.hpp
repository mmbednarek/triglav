#pragma once

#include "triglav/Int.hpp"
#include "triglav/io/Stream.hpp"

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

}// namespace triglav::gltf
