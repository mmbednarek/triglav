#include "BindlessGeometry.hpp"

#include "BindlessScene.hpp"

#include "triglav/graphics_api/PipelineBuilder.hpp"

namespace triglav::renderer::node {

namespace gapi = graphics_api;
using namespace name_literals;
using gapi::AttachmentAttribute;

BindlessGeometryResources::BindlessGeometryResources(graphics_api::Device& device) :
    m_uniformBuffer(device)
{
}

graphics_api::UniformBuffer<UniformViewProperties>& BindlessGeometryResources::view_properties()
{
   return m_uniformBuffer;
}

BindlessGeometry::BindlessGeometry(graphics_api::Device& device, BindlessScene& bindlessScene, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_bindlessScene(bindlessScene),
    m_renderTarget(GAPI_CHECK(
       graphics_api::RenderTargetBuilder(device)
          .attachment("albedo"_name, AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
                      GAPI_FORMAT(RGBA, Float16))
          .attachment("position"_name, AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
                      GAPI_FORMAT(RGBA, Float16))
          .attachment("normal"_name, AttachmentAttribute::Color | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage,
                      GAPI_FORMAT(RGBA, Float16))
          .attachment("depth"_name,
                      AttachmentAttribute::Depth | AttachmentAttribute::ClearImage | AttachmentAttribute::StoreImage |
                         AttachmentAttribute::TransferSrc,
                      GAPI_FORMAT(D, UNorm16))
          .build())),
    m_pipeline(GAPI_CHECK(graphics_api::GraphicsPipelineBuilder(device, m_renderTarget)
                             .fragment_shader(resourceManager.get("bindless_geometry.fshader"_rc))
                             .vertex_shader(resourceManager.get("bindless_geometry.vshader"_rc))
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
                                                 graphics_api::PipelineStage::VertexShader)// 0 - View Properties (Vertex)
                             .descriptor_binding(graphics_api::DescriptorType::StorageBuffer,
                                                 graphics_api::PipelineStage::VertexShader)// 1 - Scene Meshes (Vertex)
                             // .descriptor_binding_array(graphics_api::DescriptorType::ImageSampler,
                             // graphics_api::PipelineStage::FragmentShader, 1) // 2 - Material Textures (Frag)
                             .descriptor_binding(graphics_api::DescriptorType::StorageBuffer,
                                                 graphics_api::PipelineStage::FragmentShader)// 3 - MT_SolidColor Props
                             .build()))
{
}

gapi::WorkTypeFlags BindlessGeometry::work_types() const
{
   return gapi::WorkType::Graphics;
}

void BindlessGeometry::record_commands(render_core::FrameResources& frameResources, render_core::NodeFrameResources& nodeResources,
                                       gapi::CommandList& cmdList)
{
   m_bindlessScene.on_update_scene(cmdList);

   auto& bindlessGeoResources = dynamic_cast<BindlessGeometryResources&>(nodeResources);

   std::array<graphics_api::ClearValue, 4> clearValues{
      graphics_api::ColorPalette::Black,
      graphics_api::ColorPalette::Black,
      graphics_api::ColorPalette::Black,
      graphics_api::DepthStenctilValue{1.0f, 0},
   };
   cmdList.begin_render_pass(nodeResources.framebuffer("gbuffer"_name), clearValues);

   cmdList.bind_pipeline(m_pipeline);

   cmdList.bind_vertex_buffer(m_bindlessScene.combined_vertex_buffer(), 0);
   cmdList.bind_index_buffer(m_bindlessScene.combined_index_buffer());

   bindlessGeoResources.view_properties()->view = m_bindlessScene.scene().camera().view_matrix();
   bindlessGeoResources.view_properties()->proj = m_bindlessScene.scene().camera().projection_matrix();
   bindlessGeoResources.view_properties()->normal = m_bindlessScene.scene().camera().view_matrix();
   cmdList.bind_uniform_buffer(0, bindlessGeoResources.view_properties());
   cmdList.bind_storage_buffer(1, m_bindlessScene.scene_object_buffer());
   cmdList.bind_storage_buffer(2, m_bindlessScene.material_template_properties(0));

   cmdList.draw_indirect_with_count(m_bindlessScene.scene_object_buffer(), m_bindlessScene.count_buffer(), 256,
                                    sizeof(BindlessSceneObject));

   cmdList.end_render_pass();
}

std::unique_ptr<render_core::NodeFrameResources> BindlessGeometry::create_node_resources()
{
   auto resources = std::make_unique<BindlessGeometryResources>(m_device);
   resources->add_render_target("gbuffer"_name, m_renderTarget);
   return resources;
}

}// namespace triglav::renderer::node
