#pragma once

#include "triglav/Name.hpp"
#include "triglav/graphics_api/DescriptorArray.hpp"
#include "triglav/graphics_api/QueryPool.hpp"
#include "triglav/graphics_api/Texture.hpp"

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

   explicit ResourceStorage(graphics_api::Device& device);

   void register_texture(Name name, u32 frameIndex, graphics_api::Texture&& texture);
   graphics_api::Texture& texture(Name name, u32 frameIndex);

   void register_texture_mip_view(Name name, u32 mipIndex, u32 frameIndex, graphics_api::TextureView&& textureView);
   graphics_api::TextureView& texture_mip_view(Name name, u32 mipIndex, u32 frameIndex);

   void register_buffer(Name name, u32 frameIndex, graphics_api::Buffer&& buffer);
   graphics_api::Buffer& buffer(Name name, u32 frameIndex);

   [[nodiscard]] graphics_api::QueryPool& timestamps();
   [[nodiscard]] graphics_api::QueryPool& pipeline_stats();

 private:
   std::unordered_map<ResourceID, graphics_api::Texture> m_textures;
   std::unordered_map<ResourceID, graphics_api::TextureView> m_textureMipViews;
   std::unordered_map<ResourceID, graphics_api::Buffer> m_buffers;
   graphics_api::QueryPool m_timestamps;
   graphics_api::QueryPool m_pipelineStats;
};

}// namespace triglav::render_core
