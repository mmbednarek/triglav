#pragma once

#include "DescriptorView.hpp"
#include "GraphicsApi.hpp"
#include "triglav/ObjectPool.hpp"

#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

namespace triglav::graphics_api {

class Buffer;
class Texture;
class TextureView;
class Sampler;
class Device;
class DescriptorView;

namespace ray_tracing {
class AccelerationStructure;
}

constexpr auto g_max_binding{16};

class DescriptorWriter
{
 public:
   DescriptorWriter(Device& device, const DescriptorView& desc_view);
   explicit DescriptorWriter(Device& device);
   ~DescriptorWriter();

   DescriptorWriter(const DescriptorWriter& other) = delete;
   DescriptorWriter& operator=(const DescriptorWriter& other) = delete;

   DescriptorWriter(DescriptorWriter&& other) noexcept;
   DescriptorWriter& operator=(DescriptorWriter&& other) noexcept;

   void set_storage_buffer(uint32_t binding, const Buffer& buffer);
   void set_storage_buffer(uint32_t binding, const Buffer& buffer, u32 offset, u32 size);
   void set_raw_uniform_buffer(uint32_t binding, const Buffer& buffer);
   void set_raw_uniform_buffer(u32 binding, const Buffer& buffer, u32 offset, u32 size);
   void set_uniform_buffer_array(uint32_t binding, std::span<const Buffer*> buffers);
   void set_acceleration_structure(uint32_t binding, const ray_tracing::AccelerationStructure& acc_struct);

   template<typename TValue>
   void set_uniform_buffer(const uint32_t binding, const TValue& buffer)
   {
      this->set_raw_uniform_buffer(binding, buffer.buffer());
   }

   void set_sampled_texture(uint32_t binding, const Texture& texture, const Sampler& sampler);
   void set_texture_only(uint32_t binding, const Texture& texture);
   void set_texture_view_only(uint32_t binding, const TextureView& texture);
   void set_texture_array(uint32_t binding, std::span<const Texture*> textures);
   void set_storage_image(uint32_t binding, const Texture& texture);
   void set_storage_image_view(uint32_t binding, const TextureView& texture);
   void reset_count();

   [[nodiscard]] VkDescriptorSet vulkan_descriptor_set() const;
   [[nodiscard]] std::span<VkWriteDescriptorSet> vulkan_descriptor_writes();

 private:
   void update();
   VkWriteDescriptorSet& write_binding(u32 binding, VkDescriptorType desc_type);

   Device& m_device;
   VkDescriptorSet m_descriptor_set{};

   u32 m_top_binding{};
   ObjectPool<VkDescriptorBufferInfo> m_descriptor_buffer_info_pool{};
   ObjectPool<VkDescriptorImageInfo> m_descriptor_image_info_pool{};
   ObjectPool<VkWriteDescriptorSetAccelerationStructureKHR> m_descriptor_acceleration_structure_pool{};
   std::array<VkWriteDescriptorSet, g_max_binding> m_descriptor_writes{};
};

}// namespace triglav::graphics_api
