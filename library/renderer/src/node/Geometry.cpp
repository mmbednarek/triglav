#include "Geometry.h"

#include "triglav/graphics_api/Framebuffer.h"
#include "triglav/graphics_api/PipelineBuilder.h"

#include <ranges>

namespace triglav::renderer::node {

using namespace name_literals;
using graphics_api::AttachmentAttribute;

class GeometryResources : public render_core::NodeFrameResources
{
 public:
   GeometryResources(graphics_api::Device &device, resource::ResourceManager &resourceManager,
                     MaterialManager &materialManager, Scene &scene, DebugLinesRenderer &debugLinesRenderer) :
       m_device(device),
       m_resourceManager(resourceManager),
       m_sampler(resourceManager.get("linear_repeat_mlod8_aniso.sampler"_rc)),
       m_materialManager(materialManager),
       m_scene(scene),
       m_debugLinesRenderer(debugLinesRenderer),
       m_groundUniformBuffer(m_device),
       m_skyboxUniformBuffer(m_device),
       m_onAddedObjectSink(
               scene.OnObjectAddedToScene.connect<&GeometryResources::on_object_added_to_scene>(this)),
       m_onViewportChangeSink(scene.OnViewportChange.connect<&GeometryResources::on_viewport_change>(this))
   {
      m_groundUniformBuffer->model = glm::scale(glm::mat4(1), glm::vec3{200, 200, 200});
   }

   void on_object_added_to_scene(const SceneObject &object)
   {
      const auto &model = m_resourceManager.get<ResourceType::Model>(object.model);
      graphics_api::UniformBuffer<render_core::UniformBufferObject> ubo(m_device);

      const auto modelMat = object.model_matrix();
      ubo->model          = modelMat;
      ubo->normal         = glm::transpose(glm::inverse(glm::mat3(modelMat)));

      for (const auto range : model.range) {
         const auto &material = m_resourceManager.get<ResourceType::Material>(range.materialName);
      }

      m_models.emplace_back(render_core::InstancedModel{
              object.model,
              model.boundingBox,
              object.position,
              std::move(ubo),
      });

      auto &debugBoundingBox = m_debugLines.emplace_back(
              m_debugLinesRenderer.create_line_list_from_bouding_box(model.boundingBox));
      debugBoundingBox.model = modelMat;
   }

   void on_viewport_change(const graphics_api::Resolution & /*resolution*/)
   {
      m_needsUpdate = true;
   }

   void update_uniforms()
   {
      const auto cameraViewMat       = m_scene.camera().view_matrix();
      const auto cameraProjectionMat = m_scene.camera().projection_matrix();
      for (const auto &obj : m_models) {
         obj.ubo->view    = cameraViewMat;
         obj.ubo->proj    = cameraProjectionMat;
         obj.ubo->viewPos = m_scene.camera().position();
      }
   }

   void draw_model(graphics_api::CommandList &cmdList, const render_core::InstancedModel &instancedModel)
   {
      const auto &model = m_resourceManager.get<ResourceType::Model>(instancedModel.modelName);

      cmdList.bind_vertex_array(model.mesh.vertices);
      cmdList.bind_index_array(model.mesh.indices);

      for (const auto &range : model.range) {
         cmdList.bind_uniform_buffer(0, instancedModel.ubo);

         if (not m_lastMaterial.has_value() || *m_lastMaterial != range.materialName) {
            const auto &matResources = m_materialManager.material_resources(range.materialName);

            if (not m_lastMaterialTemplate.has_value() ||
                *m_lastMaterialTemplate != matResources.materialTemplate) {
               const auto &matTemplateResources =
                       m_materialManager.material_template_resources(matResources.materialTemplate);
               cmdList.bind_pipeline(matTemplateResources.pipeline);
               m_lastMaterialTemplate = matResources.materialTemplate;

               render_core::FragmentPushConstants pushConstants{
                       .viewPosition = m_scene.camera().position(),
               };
               cmdList.push_constant(graphics_api::PipelineStage::FragmentShader, pushConstants);
            }

            u32 binding = 1;

            for (const auto textureName : matResources.textures) {
               const auto &texture = m_resourceManager.get(textureName);
               cmdList.bind_texture(binding, texture, m_sampler);
               ++binding;
            }

            if (matResources.uniformBuffer.has_value()) {
               cmdList.bind_raw_uniform_buffer(binding, *matResources.uniformBuffer);
            }

            m_lastMaterial = range.materialName;
         }

         cmdList.draw_indexed_primitives(static_cast<int>(range.size), static_cast<int>(range.offset), 0);
      }
   }

   void draw_scene_models(graphics_api::CommandList &cmdList)
   {
      if (m_needsUpdate) {
         m_needsUpdate = false;
         this->update_uniforms();
      }

      m_lastMaterial.reset();
      m_lastMaterialTemplate.reset();

      for (const auto &obj : m_models) {
         if (not m_scene.camera().is_bouding_box_visible(obj.boundingBox, obj.ubo->model))
            continue;

         this->draw_model(cmdList, obj);
      }
   }

   void draw_debug_lines(graphics_api::CommandList &cmdList)
   {
      m_debugLinesRenderer.begin_render(cmdList);
      for (const auto &obj : m_debugLines) {
         m_debugLinesRenderer.draw(cmdList, obj, m_scene.camera());
      }
   }

   SkyBox::UniformBuffer &skybox_ubo()
   {
      return m_skyboxUniformBuffer;
   }

   GroundRenderer::UniformBuffer &ground_ubo()
   {
      return m_groundUniformBuffer;
   }

 private:
   graphics_api::Device &m_device;
   resource::ResourceManager &m_resourceManager;
   graphics_api::Sampler &m_sampler;
   MaterialManager &m_materialManager;
   Scene &m_scene;
   DebugLinesRenderer &m_debugLinesRenderer;
   std::vector<render_core::InstancedModel> m_models{};
   std::vector<DebugLines> m_debugLines{};
   bool m_needsUpdate{false};
   GroundRenderer::UniformBuffer m_groundUniformBuffer;
   SkyBox::UniformBuffer m_skyboxUniformBuffer;
   std::optional<MaterialName> m_lastMaterial;
   std::optional<MaterialTemplateName> m_lastMaterialTemplate;

   Scene::OnObjectAddedToSceneDel::Sink<GeometryResources> m_onAddedObjectSink;
   Scene::OnViewportChangeDel::Sink<GeometryResources> m_onViewportChangeSink;
};

Geometry::Geometry(graphics_api::Device &device, resource::ResourceManager &resourceManager, Scene &scene) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_scene(scene),
    m_renderTarget(
            GAPI_CHECK(graphics_api::RenderTargetBuilder(device)
                               .attachment("albedo"_name,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(RGBA, Float16))
                               .attachment("position"_name,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(RGBA, Float16))
                               .attachment("normal"_name,
                                           AttachmentAttribute::Color | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(RGBA, Float16))
                               .attachment("depth"_name,
                                           AttachmentAttribute::Depth | AttachmentAttribute::ClearImage |
                                                   AttachmentAttribute::StoreImage,
                                           GAPI_FORMAT(D, UNorm16))
                               .build())),
    m_materialManager(device, m_resourceManager, m_renderTarget),
    m_skybox(device, resourceManager, m_renderTarget),
    m_groundRenderer(device, m_renderTarget, resourceManager),
    m_debugLinesRenderer(device, m_renderTarget, resourceManager),
    m_timestampArray(GAPI_CHECK(device.create_timestamp_array(2)))
{
}

graphics_api::WorkTypeFlags Geometry::work_types() const
{
   return graphics_api::WorkType::Graphics;
}

void Geometry::record_commands(render_core::FrameResources &frameResources,
                               render_core::NodeFrameResources &resources, graphics_api::CommandList &cmdList)
{
   auto &geoResources = dynamic_cast<GeometryResources &>(resources);

   cmdList.reset_timestamp_array(m_timestampArray, 0, 2);
   cmdList.write_timestamp(graphics_api::PipelineStage::Entrypoint, m_timestampArray, 0);

   std::array<graphics_api::ClearValue, 4> clearValues{
           graphics_api::ColorPalette::Black,
           graphics_api::ColorPalette::Black,
           graphics_api::ColorPalette::Black,
           graphics_api::DepthStenctilValue{1.0f, 0},
   };

   auto &framebuffer = resources.framebuffer("gbuffer"_name);
   cmdList.begin_render_pass(framebuffer, clearValues);

   m_skybox.on_render(cmdList, geoResources.skybox_ubo(), m_scene.yaw(), m_scene.pitch(),
                      static_cast<float>(framebuffer.resolution().width),
                      static_cast<float>(framebuffer.resolution().height));

   m_groundRenderer.draw(cmdList, geoResources.ground_ubo(), m_scene.camera());

   geoResources.draw_scene_models(cmdList);

   if (frameResources.has_flag("debug_lines"_name)) {
      geoResources.draw_debug_lines(cmdList);
   }

   cmdList.end_render_pass();

   cmdList.write_timestamp(graphics_api::PipelineStage::End, m_timestampArray, 1);
}

std::unique_ptr<render_core::NodeFrameResources> Geometry::create_node_resources()
{
   auto result = std::make_unique<GeometryResources>(m_device, m_resourceManager, m_materialManager, m_scene,
                                                     m_debugLinesRenderer);
   result->add_render_target("gbuffer"_name, m_renderTarget);
   return result;
}

float Geometry::gpu_time() const
{
   return m_timestampArray.get_difference(0, 1);
}

}// namespace triglav::renderer::node
