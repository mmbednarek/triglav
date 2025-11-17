#include "ResourceStorage.hpp"

#include "triglav/graphics_api/Device.hpp"

namespace triglav::render_core {

namespace {

constexpr u32 g_max_timestamp_count{16};

ResourceStorage::ResourceID to_resource_id(const Name name, const u32 frame_id)
{
   return name + 82646923u * frame_id;
}

ResourceStorage::ResourceID to_resource_id(const Name name, const u32 mip_level, const u32 frame_id)
{
   return name + 82646923u * frame_id + 24318937u * mip_level;
}

}// namespace

graphics_api::DescriptorArray& DescriptorStorage::store_descriptor_array(graphics_api::DescriptorArray&& desc_array)
{
   return m_descriptor_arrays.emplace_back(std::move(desc_array));
}

ResourceStorage::ResourceStorage(graphics_api::Device& device) :
    m_timestamps(GAPI_CHECK(device.create_query_pool(graphics_api::QueryType::Timestamp, g_max_timestamp_count))),
    m_pipeline_stats(GAPI_CHECK(device.create_query_pool(graphics_api::QueryType::PipelineStats, g_max_timestamp_count)))
{
}

void ResourceStorage::register_texture(const Name name, const u32 frame_index, graphics_api::Texture&& texture)
{
   const auto res_name = to_resource_id(name, frame_index);
   if (m_textures.contains(res_name)) {
      m_textures.erase(res_name);
   }
   m_textures.emplace(to_resource_id(name, frame_index), std::move(texture));
}

graphics_api::Texture& ResourceStorage::texture(const Name name, const u32 frame_index)
{
   return m_textures.at(to_resource_id(name, frame_index));
}

void ResourceStorage::register_texture_mip_view(const Name name, const u32 mip_index, const u32 frame_index,
                                                graphics_api::TextureView&& texture_view)
{
   const auto res_name = to_resource_id(name, mip_index, frame_index);
   if (m_texture_mip_views.contains(res_name)) {
      m_texture_mip_views.erase(res_name);
   }
   m_texture_mip_views.emplace(res_name, std::move(texture_view));
}

graphics_api::TextureView& ResourceStorage::texture_mip_view(const Name name, const u32 mip_index, const u32 frame_index)
{
   return m_texture_mip_views.at(to_resource_id(name, mip_index, frame_index));
}

void ResourceStorage::register_buffer(const Name name, const u32 frame_index, graphics_api::Buffer&& buffer)
{
   const auto res_name = to_resource_id(name, frame_index);
   if (m_buffers.contains(res_name)) {
      m_buffers.erase(res_name);
   }
   m_buffers.emplace(res_name, std::move(buffer));
}

graphics_api::Buffer& ResourceStorage::buffer(const Name name, const u32 frame_index)
{
   return m_buffers.at(to_resource_id(name, frame_index));
}

graphics_api::QueryPool& ResourceStorage::timestamps()
{
   return m_timestamps;
}

graphics_api::QueryPool& ResourceStorage::pipeline_stats()
{
   return m_pipeline_stats;
}

}// namespace triglav::render_core