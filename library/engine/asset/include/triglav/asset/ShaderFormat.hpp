#pragma once

#include "triglav/Int.hpp"
#include "triglav/Name.hpp"
#include "triglav/meta/Meta.hpp"

#include <vector>

namespace triglav::io {
class Path;
}

namespace triglav::asset {

enum class BytecodeFormat
{
   SPIRV,
   DXIL,
   MSL,
   MaxValue,
};

struct ShaderBlob
{
   TG_META_STRUCT_BODY(ShaderBlob)

   BytecodeFormat format;
   std::vector<u8> data;
};

enum class ShaderPropertyType
{
   Texture2D,
   RWTexture2D,
   Sampler2D,
   SamplerState,
   ConstantBuffer,
   StructuredBuffer,
   RWStructuredBuffer,
   AccelerationStructure,
};

struct ShaderProperty
{
   TG_META_STRUCT_BODY(ShaderProperty)

   Name name;
   ShaderPropertyType type;
   bool is_array;

   u32 vulkan_descriptor_set;
   u32 vulkan_descriptor_binding;

   u32 d3d12_heap_offset;
};

struct ShaderData
{
   TG_META_STRUCT_BODY(ShaderData)

   std::vector<ShaderProperty> properties;
   std::vector<ShaderBlob> blobs;
};

std::optional<ShaderData> load_shader_data(const io::Path& path);

}// namespace triglav::asset
