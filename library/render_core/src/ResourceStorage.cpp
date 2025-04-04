#include "ResourceStorage.hpp"

#include "triglav/graphics_api/Device.hpp"

namespace triglav::render_core {

namespace {

constexpr u32 g_maxTimestampCount{16};

ResourceStorage::ResourceID to_resource_id(const Name name, const u32 frameID)
{
   return name + 82646923u * frameID;
}

ResourceStorage::ResourceID to_resource_id(const Name name, const u32 mipLevel, const u32 frameID)
{
   return name + 82646923u * frameID + 24318937u * mipLevel;
}

}// namespace

graphics_api::DescriptorArray& DescriptorStorage::store_descriptor_array(graphics_api::DescriptorArray&& descArray)
{
   return m_descriptorArrays.emplace_back(std::move(descArray));
}

ResourceStorage::ResourceStorage(graphics_api::Device& device) :
    m_timestamps(GAPI_CHECK(device.create_query_pool(graphics_api::QueryType::Timestamp, g_maxTimestampCount))),
    m_pipelineStats(GAPI_CHECK(device.create_query_pool(graphics_api::QueryType::PipelineStats, g_maxTimestampCount)))
{
}

void ResourceStorage::register_texture(const Name name, const u32 frameIndex, graphics_api::Texture&& texture)
{
   const auto resName = to_resource_id(name, frameIndex);
   if (m_textures.contains(resName)) {
      m_textures.erase(resName);
   }
   m_textures.emplace(to_resource_id(name, frameIndex), std::move(texture));
}

graphics_api::Texture& ResourceStorage::texture(const Name name, const u32 frameIndex)
{
   return m_textures.at(to_resource_id(name, frameIndex));
}

void ResourceStorage::register_texture_mip_view(const Name name, const u32 mipIndex, const u32 frameIndex,
                                                graphics_api::TextureView&& textureView)
{
   const auto resName = to_resource_id(name, mipIndex, frameIndex);
   if (m_textureMipViews.contains(resName)) {
      m_textureMipViews.erase(resName);
   }
   m_textureMipViews.emplace(resName, std::move(textureView));
}

graphics_api::TextureView& ResourceStorage::texture_mip_view(const Name name, const u32 mipIndex, const u32 frameIndex)
{
   return m_textureMipViews.at(to_resource_id(name, mipIndex, frameIndex));
}

void ResourceStorage::register_buffer(const Name name, const u32 frameIndex, graphics_api::Buffer&& buffer)
{
   const auto resName = to_resource_id(name, frameIndex);
   if (m_buffers.contains(resName)) {
      m_buffers.erase(resName);
   }
   m_buffers.emplace(resName, std::move(buffer));
}

graphics_api::Buffer& ResourceStorage::buffer(const Name name, const u32 frameIndex)
{
   return m_buffers.at(to_resource_id(name, frameIndex));
}

graphics_api::QueryPool& ResourceStorage::timestamps()
{
   return m_timestamps;
}

graphics_api::QueryPool& ResourceStorage::pipeline_stats()
{
   return m_pipelineStats;
}

}// namespace triglav::render_core