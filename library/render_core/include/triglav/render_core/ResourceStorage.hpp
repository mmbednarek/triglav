#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/Texture.hpp"

#include <triglav/graphics_api/DescriptorArray.hpp>
#include <unordered_map>

namespace triglav::render_core {

class DescriptorStorage
{
 public:
   graphics_api::DescriptorArray& store_descriptor_array(graphics_api::DescriptorArray&& descArray);

 private:
   std::vector<graphics_api::DescriptorArray> m_descriptorArrays;
};


class ResourceStorage
{
 public:
   using ResourceID = u64;

   void register_texture(Name name, u32 frameIndex, graphics_api::Texture&& texture);
   graphics_api::Texture& texture(Name name, u32 frameIndex);

   void register_buffer(Name name, u32 frameIndex, graphics_api::Buffer&& buffer);
   graphics_api::Buffer& buffer(Name name, u32 frameIndex);

 private:
   std::unordered_map<ResourceID, graphics_api::Texture> m_textures;
   std::unordered_map<ResourceID, graphics_api::Buffer> m_buffers;
};

}// namespace triglav::render_core
