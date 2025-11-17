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
   graphics_api::DescriptorArray& store_descriptor_array(graphics_api::DescriptorArray&& desc_array);

 private:
   std::vector<graphics_api::DescriptorArray> m_descriptor_arrays;
};

class ResourceStorage
{
 public:
   using ResourceID = u64;

   explicit ResourceStorage(graphics_api::Device& device);

   void register_texture(Name name, u32 frame_index, graphics_api::Texture&& texture);
   graphics_api::Texture& texture(Name name, u32 frame_index);

   void register_texture_mip_view(Name name, u32 mip_index, u32 frame_index, graphics_api::TextureView&& texture_view);
   graphics_api::TextureView& texture_mip_view(Name name, u32 mip_index, u32 frame_index);

   void register_buffer(Name name, u32 frame_index, graphics_api::Buffer&& buffer);
   graphics_api::Buffer& buffer(Name name, u32 frame_index);

   [[nodiscard]] graphics_api::QueryPool& timestamps();
   [[nodiscard]] graphics_api::QueryPool& pipeline_stats();

 private:
   std::unordered_map<ResourceID, graphics_api::Texture> m_textures;
   std::unordered_map<ResourceID, graphics_api::TextureView> m_texture_mip_views;
   std::unordered_map<ResourceID, graphics_api::Buffer> m_buffers;
   graphics_api::QueryPool m_timestamps;
   graphics_api::QueryPool m_pipeline_stats;
};

}// namespace triglav::render_core
