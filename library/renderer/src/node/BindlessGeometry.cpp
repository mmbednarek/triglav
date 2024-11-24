#include "BindlessGeometry.hpp"

#include "BindlessScene.hpp"

#include "triglav/graphics_api/PipelineBuilder.hpp"

namespace triglav::renderer::node {

namespace gapi = graphics_api;
using namespace name_literals;
using gapi::AttachmentAttribute;

static constexpr u32 upper_div(const u32 nom, const u32 denom)
{
   if ((nom % denom) == 0)
      return nom / denom;
   return 1 + (nom / denom);
}

BindlessGeometryResources::BindlessGeometryResources(graphics_api::Device& device) :
    m_uniformBuffer(device),
    m_groundUniformBuffer(device),
    m_visibleObjects(device, 128, gapi::BufferUsage::Indirect),
    m_countBuffer(GAPI_CHECK(
       device.create_buffer(gapi::BufferUsage::StorageBuffer | gapi::BufferUsage::Indirect | gapi::BufferUsage::TransferDst, sizeof(u32)))),
    m_hiZBuffer(GAPI_CHECK(device.create_texture(GAPI_FORMAT(D, UNorm16), gapi::Resolution{960, 540},
                                                 gapi::TextureUsage::Storage | gapi::TextureUsage::DepthStencilAttachment,
                                                 gapi::TextureState::Undefined, gapi::SampleCount::Single, 9))),
    m_hiZBufferScratch(GAPI_CHECK(device.create_texture(GAPI_FORMAT(D, UNorm16), gapi::Resolution{960, 540},
                                                        gapi::TextureUsage::Storage | gapi::TextureUsage::DepthStencilAttachment)))
{
}

graphics_api::UniformBuffer<UniformViewProperties>& BindlessGeometryResources::view_properties()
{
   return m_uniformBuffer;
}

GroundRenderer::UniformBuffer& BindlessGeometryResources::ground_ubo()
{
   return m_groundUniformBuffer;
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
    m_depthPrepassRenderTarget(GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                                             .attachment("depth_prepass"_name,
                                                         AttachmentAttribute::Depth | AttachmentAttribute::ClearImage |
                                                            AttachmentAttribute::StoreImage | AttachmentAttribute::TransferSrc,
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
                             .build())),
    m_depthPrepassPipeline(GAPI_CHECK(graphics_api::GraphicsPipelineBuilder(device, m_depthPrepassRenderTarget)
                                         .fragment_shader(resourceManager.get("bindless_geometry_depth_prepass.fshader"_rc))
                                         .vertex_shader(resourceManager.get("bindless_geometry_depth_prepass.vshader"_rc))
                                         .enable_depth_test(true)
                                         .enable_blending(false)
                                         .use_push_descriptors(true)
                                         // Vertex description
                                         .begin_vertex_layout<geometry::Vertex>()
                                         .vertex_attribute(GAPI_FORMAT(RGB, Float32), offsetof(geometry::Vertex, location))
                                         .end_vertex_layout()
                                         // Descriptor layout
                                         .descriptor_binding(graphics_api::DescriptorType::UniformBuffer,
                                                             graphics_api::PipelineStage::VertexShader)// 0 - View Properties (Vertex)
                                         .descriptor_binding(graphics_api::DescriptorType::StorageBuffer,
                                                             graphics_api::PipelineStage::VertexShader)// 1 - Scene Meshes (Vertex)
                                         .build())),
    m_cullingPipeline(GAPI_CHECK(graphics_api::ComputePipelineBuilder(device)
                                    .compute_shader(resourceManager.get("bindless_geometry_culling.cshader"_rc))
                                    .use_push_descriptors(true)
                                    .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                                    .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                                    .descriptor_binding(gapi::DescriptorType::StorageBuffer)
                                    .descriptor_binding(gapi::DescriptorType::UniformBuffer)
                                    .descriptor_binding(gapi::DescriptorType::ImageOnly)
                                    .build())),
    m_hiZBufferPipeline(GAPI_CHECK(graphics_api::ComputePipelineBuilder(device)
                                      .compute_shader(resourceManager.get("bindless_geometry_hi_zbuffer.cshader"_rc))
                                      .use_push_descriptors(true)
                                      .descriptor_binding(gapi::DescriptorType::ImageOnly)
                                      .descriptor_binding(gapi::DescriptorType::StorageImage)
                                      .push_constant(gapi::PipelineStage::ComputeShader, sizeof(int))
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

   const auto& camera = m_bindlessScene.scene().camera();
   bindlessGeoResources.view_properties()->view = camera.view_matrix();
   bindlessGeoResources.view_properties()->proj = camera.projection_matrix();
   bindlessGeoResources.view_properties()->nearPlane = camera.near_plane();
   bindlessGeoResources.view_properties()->farPlane = camera.far_plane();

   // -- RENDER DEPTH PREPASS --

   std::array<graphics_api::ClearValue, 1> depthPrepassClearValues{
      graphics_api::DepthStenctilValue{1.0f, 0},
   };
   cmdList.begin_render_pass(nodeResources.framebuffer("depth_prepass"_name), depthPrepassClearValues);
   cmdList.bind_pipeline(m_depthPrepassPipeline);

   cmdList.bind_vertex_buffer(m_bindlessScene.combined_vertex_buffer(), 0);
   cmdList.bind_index_buffer(m_bindlessScene.combined_index_buffer());

   cmdList.bind_uniform_buffer(0, bindlessGeoResources.view_properties());
   cmdList.bind_storage_buffer(1, m_bindlessScene.scene_object_buffer());

   cmdList.draw_indirect_with_count(m_bindlessScene.scene_object_buffer(), m_bindlessScene.count_buffer(), 100,
                                    sizeof(BindlessSceneObject));

   cmdList.end_render_pass();

   gapi::TextureBarrierInfo depthPrepassBarrier{
      .texture = &bindlessGeoResources.m_hiZBufferScratch,
      .sourceState = gapi::TextureState::Undefined,
      .targetState = gapi::TextureState::General,
      .baseMipLevel = 0,
      .mipLevelCount = 1,
   };
   cmdList.texture_barrier(gapi::PipelineStage::FragmentShader, gapi::PipelineStage::ComputeShader, depthPrepassBarrier);

   // -- CONSTRUCT HI-ZBUFFER --

   cmdList.bind_pipeline(m_hiZBufferPipeline);

   cmdList.bind_texture_image(0, nodeResources.texture("depth_prepass"_name));
   cmdList.bind_storage_image(1, bindlessGeoResources.m_hiZBufferScratch);

   int mipLevel = 0;
   cmdList.push_constant(gapi::PipelineStage::ComputeShader, mipLevel);

   cmdList.dispatch(upper_div(960, 32), upper_div(540, 32), 1);

   std::array depthPrepassBarrier2{gapi::TextureBarrierInfo{
                                      .texture = &bindlessGeoResources.m_hiZBufferScratch,
                                      .sourceState = gapi::TextureState::General,
                                      .targetState = gapi::TextureState::TransferSrc,
                                      .baseMipLevel = 0,
                                      .mipLevelCount = 1,
                                   },
                                   gapi::TextureBarrierInfo{
                                      .texture = &bindlessGeoResources.m_hiZBuffer,
                                      .sourceState = gapi::TextureState::Undefined,
                                      .targetState = gapi::TextureState::TransferDst,
                                      .baseMipLevel = 1,
                                      .mipLevelCount = 1,
                                   }};
   cmdList.texture_barrier(gapi::PipelineStage::ComputeShader, gapi::PipelineStage::Transfer, depthPrepassBarrier2);

   cmdList.copy_texture(bindlessGeoResources.m_hiZBufferScratch, gapi::TextureState::TransferSrc, bindlessGeoResources.m_hiZBuffer,
                        gapi::TextureState::TransferDst, 0, 0);

   int depthWidth = 480;
   int depthHeight = 270;
   for (int mipLevel = 0; mipLevel < 8; ++mipLevel) {
      std::array depthPrepassBarrier3{gapi::TextureBarrierInfo{
                                         .texture = &bindlessGeoResources.m_hiZBufferScratch,
                                         .sourceState = gapi::TextureState::TransferSrc,
                                         .targetState = gapi::TextureState::General,
                                         .baseMipLevel = 0,
                                         .mipLevelCount = 1,
                                      },
                                      gapi::TextureBarrierInfo{
                                         .texture = &bindlessGeoResources.m_hiZBuffer,
                                         .sourceState = gapi::TextureState::TransferDst,
                                         .targetState = gapi::TextureState::DepthStencilRead,
                                         .baseMipLevel = mipLevel,
                                         .mipLevelCount = 1,
                                      }};
      cmdList.texture_barrier(gapi::PipelineStage::Transfer, gapi::PipelineStage::ComputeShader, depthPrepassBarrier3);

      cmdList.bind_texture_image(0, bindlessGeoResources.m_hiZBuffer);
      cmdList.bind_storage_image(1, bindlessGeoResources.m_hiZBufferScratch);

      cmdList.push_constant(gapi::PipelineStage::ComputeShader, mipLevel);

      cmdList.dispatch(upper_div(depthWidth, 32), upper_div(depthHeight, 32), 1);
      depthWidth /= 2;
      depthHeight /= 2;

      std::array depthPrepassBarrier4{gapi::TextureBarrierInfo{
                                         .texture = &bindlessGeoResources.m_hiZBufferScratch,
                                         .sourceState = gapi::TextureState::General,
                                         .targetState = gapi::TextureState::TransferSrc,
                                         .baseMipLevel = 0,
                                         .mipLevelCount = 1,
                                      },
                                      gapi::TextureBarrierInfo{
                                         .texture = &bindlessGeoResources.m_hiZBuffer,
                                         .sourceState = gapi::TextureState::Undefined,
                                         .targetState = gapi::TextureState::TransferDst,
                                         .baseMipLevel = mipLevel + 1,
                                         .mipLevelCount = 1,
                                      }};
      cmdList.texture_barrier(gapi::PipelineStage::ComputeShader, gapi::PipelineStage::Transfer, depthPrepassBarrier4);

      cmdList.copy_texture(bindlessGeoResources.m_hiZBufferScratch, gapi::TextureState::TransferSrc, bindlessGeoResources.m_hiZBuffer,
                           gapi::TextureState::TransferDst, 0, mipLevel + 1);
   }

   gapi::TextureBarrierInfo depthPrepassBarrier5{
      .texture = &bindlessGeoResources.m_hiZBuffer,
      .sourceState = gapi::TextureState::TransferDst,
      .targetState = gapi::TextureState::DepthStencilRead,
      .baseMipLevel = 0,
      .mipLevelCount = 9,
   };
   cmdList.texture_barrier(gapi::PipelineStage::ComputeShader, gapi::PipelineStage::ComputeShader, depthPrepassBarrier5);

   // -- CULL SCENE OBJECTS --

   constexpr u32 initialCount = 0;
   cmdList.update_buffer(bindlessGeoResources.m_countBuffer, 0, sizeof(u32), &initialCount);
   cmdList.execution_barrier(graphics_api::PipelineStage::Transfer, graphics_api::PipelineStage::ComputeShader);

   cmdList.bind_pipeline(m_cullingPipeline);
   cmdList.bind_storage_buffer(0, m_bindlessScene.scene_object_buffer());
   cmdList.bind_storage_buffer(1, bindlessGeoResources.m_visibleObjects.buffer());
   cmdList.bind_storage_buffer(2, bindlessGeoResources.m_countBuffer);
   cmdList.bind_uniform_buffer(3, bindlessGeoResources.view_properties());
   cmdList.bind_texture_image(4, bindlessGeoResources.m_hiZBuffer);
   cmdList.dispatch(upper_div(m_bindlessScene.scene_object_count(), 1024), 1, 1);

   cmdList.execution_barrier(graphics_api::PipelineStage::ComputeShader, graphics_api::PipelineStage::VertexShader);

   // -- RENDER GEOMETRY --

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

   cmdList.bind_uniform_buffer(0, bindlessGeoResources.view_properties());
   cmdList.bind_storage_buffer(1, bindlessGeoResources.m_visibleObjects.buffer());
   cmdList.bind_storage_buffer(2, m_bindlessScene.material_template_properties(0));

   cmdList.draw_indirect_with_count(bindlessGeoResources.m_visibleObjects.buffer(), bindlessGeoResources.m_countBuffer, 100,
                                    sizeof(BindlessSceneObject));

   cmdList.end_render_pass();
}

std::unique_ptr<render_core::NodeFrameResources> BindlessGeometry::create_node_resources()
{
   auto resources = std::make_unique<BindlessGeometryResources>(m_device);
   resources->add_render_target("gbuffer"_name, m_renderTarget);
   resources->add_render_target("depth_prepass"_name, m_depthPrepassRenderTarget);
   return resources;
}

}// namespace triglav::renderer::node
