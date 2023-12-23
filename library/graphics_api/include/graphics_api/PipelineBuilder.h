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
   [[nodiscard]] Result<Pipeline> build();

   template<typename TVertex>
   PipelineBuilder &begin_vertex_layout()
   {
      m_vertexLayoutSizes.push_back(sizeof(TVertex));
      return *this;
   }

 private:
   Device &m_device;
   RenderPass &m_renderPass;
   std::vector<const Shader *> m_shaders;
   std::vector<VertexInputAttribute> m_vertexAttributes;
   std::vector<VertexInputLayout> m_vertexLayouts;
   std::vector<DescriptorBinding> m_descriptorBindings;
   std::vector<PushConstant> m_pushConstants;
   std::vector<size_t> m_vertexLayoutSizes{};
   uint32_t m_vertexLocation{0};
   bool m_depthTestEnabled{false};
};

}// namespace graphics_api