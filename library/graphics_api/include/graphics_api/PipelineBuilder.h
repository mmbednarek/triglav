#pragma once

#include "GraphicsApi.hpp"
#include "Pipeline.h"

#include <tuple>
#include <vector>

namespace graphics_api {

class Device;
class RenderPass;
class Shader;

class PipelineBuilder
{
 public:
   PipelineBuilder(Device &device, RenderPass &renderPass);

   PipelineBuilder &fragment_shader(const Shader &shader);
   PipelineBuilder &vertex_shader(const Shader &shader);
   PipelineBuilder &vertex_attribute(const ColorFormat &format, size_t offset);
   PipelineBuilder &end_vertex_layout();
   PipelineBuilder &descriptor_binding(DescriptorType descriptorType, ShaderStage shaderStage);
   PipelineBuilder &push_constant(ShaderStage shaderStage, size_t size, size_t offset = 0);
   PipelineBuilder &enable_depth_test(bool enabled);

   [[nodiscard]] Result<Pipeline> new_build() const;

   template<typename TVertex>
   PipelineBuilder &begin_vertex_layout()
   {
      VkVertexInputBindingDescription binding{};
      binding.binding   = m_vertexBinding;
      binding.stride    = sizeof(TVertex);
      binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      m_bindings.emplace_back(binding);
      return *this;
   }

 private:
   Device &m_device;
   RenderPass &m_renderPass;

   uint32_t m_vertexLocation{0};
   uint32_t m_vertexBinding{0};
   bool m_depthTestEnabled{false};

   std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
   std::vector<VkVertexInputBindingDescription> m_bindings{};
   std::vector<VkVertexInputAttributeDescription> m_attributes{};
   std::vector<VkDescriptorSetLayoutBinding> m_vulkanDescriptorBindings{};
   std::vector<VkPushConstantRange> m_pushConstantRanges{};
};

}// namespace graphics_api