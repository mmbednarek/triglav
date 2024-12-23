#include "MaterialManager.hpp"

#include "triglav/geometry/Geometry.hpp"
#include "triglav/graphics_api/Device.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"
#include "triglav/io/BufferWriter.hpp"
#include "triglav/io/Serializer.hpp"
#include "triglav/render_core/Material.hpp"
#include "triglav/render_core/Model.hpp"

namespace triglav::renderer {

using graphics_api::BufferUsage;

MaterialManager::MaterialManager(graphics_api::Device& device, resource::ResourceManager& resourceManager,
                                 graphics_api::RenderTarget& renderTarget) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_renderTarget(renderTarget)
{
   m_resourceManager.iterate_resources<ResourceType::MaterialTemplate>(
      [this](MaterialTemplateName name, const render_core::MaterialTemplate& material) {
         this->process_material_template(name, material);
      });

   m_resourceManager.iterate_resources<ResourceType::Material>(
      [this](MaterialName name, const render_core::Material& material) { this->process_material(name, material); });
}

void MaterialManager::process_material(const MaterialName name, const render_core::Material& material)
{
   std::array<u8, 1024> buffer{};
   io::BufferWriter writer(buffer);
   io::Serializer serializer(writer);

   std::vector<TextureName> textures;

   for (const auto& value : material.values) {
      if (std::holds_alternative<TextureName>(value)) {
         textures.emplace_back(std::get<TextureName>(value));
      } else if (std::holds_alternative<float>(value)) {
         serializer.write_float32(std::get<float>(value));
      } else if (std::holds_alternative<glm::vec3>(value)) {
         serializer.write_vec3(std::get<glm::vec3>(value));
      } else if (std::holds_alternative<glm::vec4>(value)) {
         serializer.write_vec4(std::get<glm::vec4>(value));
      } else if (std::holds_alternative<glm::mat4>(value)) {
         serializer.write_mat4(std::get<glm::mat4>(value));
      }
   }

   std::optional<graphics_api::Buffer> constantsBuffer{};
   if (writer.offset() != 0) {
      constantsBuffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::TransferDst | BufferUsage::UniformBuffer, writer.offset()));
      GAPI_CHECK_STATUS(constantsBuffer->write_indirect(buffer.data(), writer.offset()));
   }

   u32 worldDataUboSize = 0;
   auto& matTem = m_resourceManager.get(material.materialTemplate);
   for (const auto& prop : matTem.properties) {
      if (prop.source == render_core::PropertySource::Constant)
         continue;

      switch (prop.type) {
      case render_core::MaterialPropertyType::Float32:
         worldDataUboSize += sizeof(float);
      case render_core::MaterialPropertyType::Vector3: {
         const auto mod = worldDataUboSize % (4 * sizeof(float));
         if (mod != 0) {
            auto padding = 4 * sizeof(float) - mod;
            worldDataUboSize += padding;
         }
         worldDataUboSize += 3 * sizeof(float);
         break;
      }
      case render_core::MaterialPropertyType::Vector4: {
         const auto mod = worldDataUboSize % (4 * sizeof(float));
         if (mod != 0) {
            auto padding = 4 * sizeof(float) - mod;
            worldDataUboSize += padding;
         }
         worldDataUboSize += 4 * sizeof(glm::vec4);
         break;
      }
      case render_core::MaterialPropertyType::Matrix4x4: {
         const auto mod = worldDataUboSize % (4 * sizeof(float));
         if (mod != 0) {
            auto padding = 4 * sizeof(float) - mod;
            worldDataUboSize += padding;
         }
         worldDataUboSize += 16 * sizeof(glm::vec4);
         break;
      }
      }
   }

   std::optional<graphics_api::Buffer> worldDataBuffer{};
   if (worldDataUboSize != 0) {
      worldDataBuffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::HostVisible | BufferUsage::UniformBuffer, worldDataUboSize));
   }

   m_materials.emplace(name, MaterialResources{
                                .materialTemplate{material.materialTemplate},
                                .constantsUniformBuffer{std::move(constantsBuffer)},
                                .worldDataUniformBuffer{std::move(worldDataBuffer)},
                                .textures{std::move(textures)},
                             });
}

void MaterialManager::process_material_template(const MaterialTemplateName name, const render_core::MaterialTemplate& materialTemplate)
{
   auto builder = graphics_api::GraphicsPipelineBuilder(m_device, m_renderTarget)
                     .fragment_shader(m_resourceManager.get(materialTemplate.fragmentShader))
                     .vertex_shader(m_resourceManager.get(materialTemplate.vertexShader))
                     .enable_depth_test(true)
                     .enable_blending(false)
                     .use_push_descriptors(true)
                     // Vertex description
                     .begin_vertex_layout<geometry::Vertex>()
                     .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                     .vertex_attribute(GAPI_FORMAT(RG, Float32), offsetof(geometry::Vertex, uv))
                     .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, normal))
                     .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, tangent))
                     .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, bitangent))
                     .end_vertex_layout()
                     // Descriptor layout
                     .descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::VertexShader);

   bool hasConstantsUbo = false;
   bool hasWorldDataUbo = false;
   for (const auto& property : materialTemplate.properties) {
      switch (property.type) {
      case render_core::MaterialPropertyType::Texture2D:
         builder.descriptor_binding(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader);
         break;
      case render_core::MaterialPropertyType::Float32:
         [[fallthrough]];
      case render_core::MaterialPropertyType::Vector3:
         [[fallthrough]];
      case render_core::MaterialPropertyType::Vector4:
         [[fallthrough]];
      case render_core::MaterialPropertyType::Matrix4x4:
         if (property.source == render_core::PropertySource::Constant) {
            hasConstantsUbo = true;
         } else {
            hasWorldDataUbo = true;
         }
         break;
      }
   }

   if (hasConstantsUbo) {
      builder.descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::FragmentShader);
   }
   if (hasWorldDataUbo) {
      builder.descriptor_binding(graphics_api::DescriptorType::UniformBuffer, graphics_api::PipelineStage::FragmentShader);
   }

   m_templates.emplace(name, MaterialTemplateResources{.pipeline = GAPI_CHECK(builder.build())});
}

MaterialResources& MaterialManager::material_resources(const MaterialName name)
{
   return m_materials.at(name);
}

const MaterialTemplateResources& MaterialManager::material_template_resources(const MaterialTemplateName name) const
{
   return m_templates.at(name);
}

}// namespace triglav::renderer
