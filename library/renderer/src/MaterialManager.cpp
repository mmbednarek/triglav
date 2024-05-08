#include "MaterialManager.h"

#include "triglav/geometry/Geometry.h"
#include "triglav/graphics_api/Device.h"
#include "triglav/graphics_api/PipelineBuilder.h"
#include "triglav/io/BufferWriter.h"
#include "triglav/io/Serializer.h"
#include "triglav/render_core/Material.hpp"

namespace triglav::renderer {

MaterialManager::MaterialManager(graphics_api::Device &device, resource::ResourceManager &resourceManager,
                                 graphics_api::RenderTarget &renderTarget) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_renderTarget(renderTarget)
{
   m_resourceManager.iterate_resources<ResourceType::MaterialTemplate>(
           [this](MaterialTemplateName name, const render_core::MaterialTemplate &material) {
             this->process_material_template(name, material);
           });

   m_resourceManager.iterate_resources<ResourceType::Material>(
           [this](MaterialName name, const render_core::Material &material) {
             this->process_material(name, material);
           });
}

void MaterialManager::process_material(const MaterialName name, const render_core::Material &material)
{
   std::array<u8, 1024> buffer{};
   io::BufferWriter writer(buffer);
   io::Serializer serializer(writer);

   std::vector<TextureName> textures;

   for (const auto &value : material.values) {
      if (std::holds_alternative<TextureName>(value)) {
         textures.emplace_back(std::get<TextureName>(value));
      } else if (std::holds_alternative<float>(value)) {
         serializer.write_float32(std::get<float>(value));
      } else if (std::holds_alternative<glm::vec3>(value)) {
         serializer.write_vec3(std::get<glm::vec3>(value));
      } else if (std::holds_alternative<glm::vec4>(value)) {
         serializer.write_vec4(std::get<glm::vec4>(value));
      }
   }

   if (writer.offset() == 0) {
      m_materials.emplace(name, MaterialResources{.materialTemplate{material.materialTemplate}, .uniformBuffer{std::nullopt}, .textures{std::move(textures)}});
      return;
   }

   auto uniformBuffer = GAPI_CHECK(m_device.create_buffer(graphics_api::BufferPurpose::UniformBuffer, writer.offset()));
   uniformBuffer.map_memory()->write(buffer.data(), writer.offset());

   m_materials.emplace(name, MaterialResources{.materialTemplate{material.materialTemplate}, .uniformBuffer{std::move(uniformBuffer)}, .textures{std::move(textures)}});
}

void MaterialManager::process_material_template(const MaterialTemplateName name,
                                                const render_core::MaterialTemplate &materialTemplate)
{
   auto builder = graphics_api::PipelineBuilder(m_device, m_renderTarget)
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
                          .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                              graphics_api::PipelineStage::VertexShader);

   bool hasUbo = false;
   for (const auto &property : materialTemplate.properties) {
      switch (property.type) {
      case render_core::MaterialPropertyType::Texture2D:
         builder.descriptor_binding(graphics_api::DescriptorType::ImageSampler,
                                    graphics_api::PipelineStage::FragmentShader);
         break;
      case render_core::MaterialPropertyType::Float32: [[fallthrough]];
      case render_core::MaterialPropertyType::Vec3: [[fallthrough]];
      case render_core::MaterialPropertyType::Vec4: hasUbo = true; break;
      }
   }

   if (hasUbo) {
      builder.descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                 graphics_api::PipelineStage::FragmentShader);
   }

   m_templates.emplace(name, MaterialTemplateResources{.pipeline = GAPI_CHECK(builder.build())});
}

const MaterialResources &MaterialManager::material_resources(const MaterialName name) const
{
   return m_materials.at(name);
}

const MaterialTemplateResources &MaterialManager::material_template_resources(const MaterialTemplateName name) const
{
   return m_templates.at(name);
}

}// namespace triglav::renderer
