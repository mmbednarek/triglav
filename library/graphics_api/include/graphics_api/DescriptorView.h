#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace graphics_api {

class Buffer;
class Texture;
class Sampler;

class DescriptorView
{
 public:
   DescriptorView(VkDevice device, VkDescriptorSet descriptorSet, VkPipelineLayout pipelineLayout);

   void reset();
   void set_raw_uniform_buffer(uint32_t binding, const Buffer &buffer);

   template<typename TValue>
   void set_uniform_buffer(const uint32_t binding, const TValue &buffer)
   {
      this->set_raw_uniform_buffer(binding, buffer.buffer());
   }

   void set_sampled_texture(uint32_t binding, const Texture &texture, const Sampler &sampler);
   void update();

   [[nodiscard]] VkDescriptorSet vulkan_descriptor_set() const;
   [[nodiscard]] VkPipelineLayout vulkan_pipeline_layout() const;

 private:
   VkDevice m_device{};
   VkDescriptorSet m_descriptorSet{};
   VkPipelineLayout m_pipelineLayout{};

   std::vector<VkDescriptorBufferInfo> m_descriptorBufferInfos{};
   std::vector<VkDescriptorImageInfo> m_descriptorImageInfos{};
   std::vector<VkWriteDescriptorSet> m_descriptorWrites{};
};

}// namespace graphics_api