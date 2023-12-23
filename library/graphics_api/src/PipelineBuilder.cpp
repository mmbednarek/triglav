#include "PipelineBuilder.h"

#include "Device.h"
#include "Shader.h"

namespace graphics_api {

PipelineBuilder::PipelineBuilder(Device &device, RenderPass &renderPass) :
    m_device(device),
    m_renderPass(renderPass)
{
}

PipelineBuilder &PipelineBuilder::fragment_shader(const Shader &shader)
{
   assert(shader.stage() == ShaderStage::Fragment);
   m_shaders.emplace_back(&shader);

   return *this;
}

PipelineBuilder &PipelineBuilder::vertex_shader(const Shader &shader)
{
   assert(shader.stage() == ShaderStage::Vertex);
   m_shaders.emplace_back(&shader);

   return *this;
}

PipelineBuilder &PipelineBuilder::vertex_attribute(const ColorFormat &format, const size_t offset)
{
   const VertexInputAttribute attribute{
           .location = m_vertexLocation,
           .format   = format,
           .offset   = offset,
   };
   m_vertexAttributes.push_back(attribute);

   ++m_vertexLocation;

   return *this;
}

PipelineBuilder &PipelineBuilder::end_vertex_layout()
{
   m_vertexLocation = 0;
   return *this;
}

PipelineBuilder &PipelineBuilder::descriptor_binding(const DescriptorType descriptorType,
                                                     ShaderStage shaderStage)
{
   const DescriptorBinding binding{
           .binding         = static_cast<int>(m_descriptorBindings.size()),
           .descriptorCount = 1,
           .type            = descriptorType,
           .shaderStages    = static_cast<ShaderStageFlags>(shaderStage),
   };
   m_descriptorBindings.push_back(binding);
   return *this;
}

PipelineBuilder &PipelineBuilder::push_constant(const ShaderStage shaderStage, size_t size, size_t offset)
{
   m_pushConstants.emplace_back(offset, size, ShaderStage::None | shaderStage);
   return *this;
}

PipelineBuilder &PipelineBuilder::enable_depth_test(const bool enabled)
{
   m_depthTestEnabled = enabled;
   return *this;
}

Result<Pipeline> PipelineBuilder::build()
{
   std::vector<VertexInputLayout> layouts{};
   layouts.resize(m_vertexLayoutSizes.size());

   size_t binding        = 0;
   size_t lastStartIndex = 0;
   size_t attributeIndex = 0;
   for (const auto &attribute : m_vertexAttributes) {
      if (attribute.location != 0) {
         ++attributeIndex;
         continue;
      }

      layouts[binding].structure_size = m_vertexLayoutSizes[binding];
      if (binding != 0) {
         layouts[binding - 1].attributes =
                 std::span{m_vertexAttributes}.subspan(lastStartIndex, attributeIndex - lastStartIndex);
      }

      lastStartIndex = attributeIndex;
      ++binding;
      ++attributeIndex;
   }
   layouts[binding - 1].attributes = std::span{m_vertexAttributes}.subspan(lastStartIndex);

   return m_device.create_pipeline(m_renderPass, m_shaders, layouts, m_descriptorBindings, m_pushConstants,
                                   m_depthTestEnabled);
}

}// namespace graphics_api