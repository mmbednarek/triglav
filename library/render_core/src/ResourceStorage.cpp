#include "ResourceStorage.hpp"

namespace triglav::render_core {

namespace {

ResourceStorage::ResourceID to_resource_id(const Name name, const u32 frameID)
{
   return name + 82646923u * frameID;
}

}// namespace

graphics_api::DescriptorArray& DescriptorStorage::store_descriptor_array(graphics_api::DescriptorArray&& descArray)
{
   return m_descriptorArrays.emplace_back(std::move(descArray));
}

void ResourceStorage::register_texture(const Name name, const u32 frameIndex, graphics_api::Texture&& texture)
{
   m_textures.emplace(to_resource_id(name, frameIndex), std::move(texture));
}

graphics_api::Texture& ResourceStorage::texture(const Name name, const u32 frameIndex)
{
   return m_textures.at(to_resource_id(name, frameIndex));
}

void ResourceStorage::register_buffer(const Name name, const u32 frameIndex, graphics_api::Buffer&& buffer)
{
   m_buffers.emplace(to_resource_id(name, frameIndex), std::move(buffer));
}

graphics_api::Buffer& ResourceStorage::buffer(const Name name, const u32 frameIndex)
{
   return m_buffers.at(to_resource_id(name, frameIndex));
}

}// namespace triglav::render_core