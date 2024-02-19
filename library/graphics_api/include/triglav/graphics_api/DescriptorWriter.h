#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <memory>

#include "DescriptorView.h"
#include "GraphicsApi.hpp"

namespace triglav::graphics_api {

class Buffer;
class Texture;
class Sampler;
class Device;
class DescriptorView;

class DescriptorWriter
{
 public:
   DescriptorWriter(const Device &device, const DescriptorView &descView);
   ~DescriptorWriter();

   DescriptorWriter(const DescriptorWriter &other)                = delete;
   DescriptorWriter(DescriptorWriter &&other) noexcept            = delete;
   DescriptorWriter &operator=(const DescriptorWriter &other)     = delete;
   DescriptorWriter &operator=(DescriptorWriter &&other) noexcept = delete;

   void set_raw_uniform_buffer(uint32_t binding, const Buffer &buffer);

   template<typename TValue>
   void set_uniform_buffer(const uint32_t binding, const TValue &buffer)
   {
      this->set_raw_uniform_buffer(binding, buffer.buffer());
   }

   void set_sampled_texture(uint32_t binding, const Texture &texture, const Sampler &sampler);

   [[nodiscard]] VkDescriptorSet vulkan_descriptor_set() const;

 private:
   void update();

   VkDevice m_device{};
   VkDescriptorSet m_descriptorSet{};

   std::vector<std::unique_ptr<VkDescriptorBufferInfo>> m_descriptorBufferInfos{};
   std::vector<std::unique_ptr<VkDescriptorImageInfo>> m_descriptorImageInfos{};
   std::vector<VkWriteDescriptorSet> m_descriptorWrites{};
};

}// namespace graphics_api
