#include "ResourceStorage.hpp"

namespace triglav::render_core {

graphics_api::DescriptorArray& DescriptorStorage::store_descriptor_array(graphics_api::DescriptorArray&& descArray)
{
   return m_descriptorArrays.emplace_back(std::move(descArray));
}

void ResourceStorage::register_texture(const Name name, graphics_api::Texture&& texture)
{
   m_textures.emplace(name, std::move(texture));
}

graphics_api::Texture& ResourceStorage::texture(const Name name)
{
   return m_textures.at(name);
}

void ResourceStorage::register_buffer(const Name name, graphics_api::Buffer&& buffer)
{
   m_buffers.emplace(name, std::move(buffer));
}

graphics_api::Buffer& ResourceStorage::buffer(const Name name)
{
   return m_buffers.at(name);
}

}// namespace triglav::render_core