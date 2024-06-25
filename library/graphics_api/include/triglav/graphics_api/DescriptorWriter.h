#pragma once

#include "DescriptorView.h"
#include "GraphicsApi.hpp"
#include "triglav/ObjectPool.hpp"

#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

namespace triglav::graphics_api {

class Buffer;
class Texture;
class Sampler;
class Device;
class DescriptorView;

constexpr auto g_maxBinding{16};

class DescriptorWriter
{
 public:
   DescriptorWriter(const Device& device, const DescriptorView& descView);
   explicit DescriptorWriter(const Device& device);
   ~DescriptorWriter();

   DescriptorWriter(const DescriptorWriter& other) = delete;
   DescriptorWriter& operator=(const DescriptorWriter& other) = delete;

   DescriptorWriter(DescriptorWriter&& other) noexcept;
   DescriptorWriter& operator=(DescriptorWriter&& other) noexcept;

   void set_storage_buffer(uint32_t binding, const Buffer& buffer);
   void set_storage_buffer(uint32_t binding, const Buffer& buffer, u32 offset, u32 size);
   void set_raw_uniform_buffer(uint32_t binding, const Buffer& buffer);

   template<typename TValue>
   void set_uniform_buffer(const uint32_t binding, const TValue& buffer)
   {
      this->set_raw_uniform_buffer(binding, buffer.buffer());
   }

   void set_sampled_texture(uint32_t binding, const Texture& texture, const Sampler& sampler);
   void reset_count();

   [[nodiscard]] VkDescriptorSet vulkan_descriptor_set() const;
   [[nodiscard]] std::span<VkWriteDescriptorSet> vulkan_descriptor_writes();

 private:
   void update();
   VkWriteDescriptorSet& write_binding(u32 binding, VkDescriptorType descType);

   VkDevice m_device{};
   VkDescriptorSet m_descriptorSet{};

   u32 m_topBinding{};
   ObjectPool<VkDescriptorBufferInfo> m_descriptorBufferInfoPool{};
   ObjectPool<VkDescriptorImageInfo> m_descriptorImageInfoPool{};
   std::array<VkWriteDescriptorSet, g_maxBinding> m_descriptorWrites{};
};

}// namespace triglav::graphics_api
