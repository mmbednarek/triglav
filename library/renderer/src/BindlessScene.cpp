#include "BindlessScene.hpp"

#include "triglav/graphics_api/Pipeline.hpp"
#include "triglav/graphics_api/PipelineBuilder.hpp"

namespace triglav::renderer {

namespace gapi = graphics_api;
using namespace name_literals;

constexpr auto STAGING_BUFFER_ELEM_COUNT = 64;
constexpr auto SCENE_ELEM_COUNT = 128;
constexpr auto VERTEX_BUFFER_SIZE = 256000;
constexpr auto INDEX_BUFFER_SIZE = 256000;

namespace {

constexpr auto encode_material_id(const u32 templateID, const u32 instanceID)
{
   return (templateID & 0b111) | (instanceID << 3);
}

}// namespace

BindlessScene::BindlessScene(gapi::Device& device, resource::ResourceManager& resourceManager, Scene& scene) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_scene(scene),
    m_sceneObjectStage(device, STAGING_BUFFER_ELEM_COUNT),
    m_sceneObjects(device, SCENE_ELEM_COUNT, gapi::BufferUsage::Indirect),
    m_combinedVertexBuffer(device, VERTEX_BUFFER_SIZE),
    m_combinedIndexBuffer(device, INDEX_BUFFER_SIZE),
    m_countBuffer(device, gapi::BufferUsage::Indirect),
    m_materialPropsAllScalar(device, 3),
    m_materialPropsAlbedoTex(device, 10),
    m_materialPropsAlbedoNormalTex(device, 10),
    TG_CONNECT(scene, OnObjectAddedToScene, on_object_added_to_scene)
{
   std::array<BindlessMaterialProps_AllScalar, 3> properties{
      BindlessMaterialProps_AllScalar{
         .albedo = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
         .roughness = 1.0,
         .metalic = 0.0,
      },
      BindlessMaterialProps_AllScalar{
         .albedo = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
         .roughness = 1.0,
         .metalic = 0.0,
      },
      BindlessMaterialProps_AllScalar{
         .albedo = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
         .roughness = 1.0,
         .metalic = 0.0,
      },
   };
   m_materialPropsAllScalar.write(properties.data(), properties.size());
}

void BindlessScene::on_object_added_to_scene(const SceneObject& object)
{
   m_pendingObjects.emplace_back(object);
}

void BindlessScene::on_update_scene(const gapi::CommandList& cmdList)
{
   if (m_pendingObjects.empty())
      return;

   {
      // TODO: Increase object buffer if necessary
      assert(m_pendingObjects.size() < STAGING_BUFFER_ELEM_COUNT);
      auto mapping{GAPI_CHECK(m_sceneObjectStage.buffer().map_memory())};
      for (const auto& pendingObject : m_pendingObjects) {
         auto& meshInfo = this->get_mesh_info(cmdList, pendingObject.model);
         // TODO: Add model if doesn't exists.

         BindlessSceneObject object;
         object.vertexOffset = meshInfo.vertexOffset;
         object.indexCount = meshInfo.indexCount;
         object.indexOffset = meshInfo.indexOffset;
         object.instanceCount = 1;
         object.instanceOffset = 0;
         object.boundingBoxMin = meshInfo.boundingBoxMin;
         object.boundingBoxMax = meshInfo.boundingBoxMax;
         object.materialID = meshInfo.materialID;
         object.transform = pendingObject.model_matrix();
         object.normalTransform = glm::transpose(glm::inverse(glm::mat3(pendingObject.model_matrix())));

         mapping.write_offset(&object, sizeof(BindlessSceneObject), m_writtenSceneObjectCount * sizeof(BindlessSceneObject));
         ++m_writtenSceneObjectCount;
      }
   }

   *m_countBuffer = m_writtenSceneObjectCount;

   cmdList.copy_buffer(m_sceneObjectStage.buffer(), m_sceneObjects.buffer(), 0, m_writtenObjectCount * sizeof(BindlessSceneObject),
                       m_pendingObjects.size() * sizeof(BindlessSceneObject));

   m_pendingObjects.clear();
}

gapi::Buffer& BindlessScene::combined_vertex_buffer()
{
   return m_combinedVertexBuffer.buffer();
}

gapi::Buffer& BindlessScene::combined_index_buffer()
{
   return m_combinedIndexBuffer.buffer();
}

gapi::Buffer& BindlessScene::scene_object_buffer()
{
   return m_sceneObjects.buffer();
}

graphics_api::Buffer& BindlessScene::material_template_properties(const u32 materialTemplateId)
{
   switch (materialTemplateId) {
   case 0:
      return m_materialPropsAllScalar.buffer();
   case 1:
      return m_materialPropsAlbedoTex.buffer();
   default:
      return m_materialPropsAlbedoNormalTex.buffer();
   }
}

const gapi::Buffer& BindlessScene::count_buffer() const
{
   return m_countBuffer.buffer();
}

u32 BindlessScene::scene_object_count() const
{
   return m_writtenSceneObjectCount;
}

Scene& BindlessScene::scene() const
{
   return m_scene;
}

graphics_api::Pipeline& BindlessScene::scene_pipeline(graphics_api::RenderTarget& renderTarget)
{
   if (!m_shouldUpdatePSO && m_scenePipeline.has_value()) {
      return *m_scenePipeline;
   }

   m_scenePipeline.emplace(
      GAPI_CHECK(graphics_api::GraphicsPipelineBuilder(m_device, renderTarget)
                    .fragment_shader(m_resourceManager.get("bindless_geometry.fshader"_rc))
                    .vertex_shader(m_resourceManager.get("bindless_geometry.vshader"_rc))
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
                    .descriptor_binding_array(graphics_api::DescriptorType::ImageSampler, graphics_api::PipelineStage::FragmentShader,
                                              m_sceneTextures.size())// 2 - Material Textures (Frag)
                    .descriptor_binding(graphics_api::DescriptorType::StorageBuffer,
                                        graphics_api::PipelineStage::FragmentShader)// 3 - MT_SolidColor Props
                    .descriptor_binding(graphics_api::DescriptorType::StorageBuffer,
                                        graphics_api::PipelineStage::FragmentShader)// 4 - MT_AlbedoTexture Props
                    .descriptor_binding(graphics_api::DescriptorType::StorageBuffer,
                                        graphics_api::PipelineStage::FragmentShader)// 5 - MT_AlbedoNormalTexture Props
                    .build()));

   m_shouldUpdatePSO = false;

   return *m_scenePipeline;
}

std::vector<graphics_api::Texture*>& BindlessScene::scene_textures()
{
   return m_sceneTextures;
}

BindlessMeshInfo& BindlessScene::get_mesh_info(const gapi::CommandList& cmdList, const ModelName name)
{
   if (const auto it = m_models.find(name); it != m_models.end()) {
      return it->second;
   }

   auto& model = m_resourceManager.get(name);

   assert(!model.range.empty());
   auto material = m_resourceManager.get(model.range[0].materialName);

   BindlessMeshInfo meshInfo;
   meshInfo.indexCount = model.mesh.indices.count();
   meshInfo.indexOffset = m_writtenIndexCount;
   meshInfo.vertexOffset = m_writtenVertexCount;
   meshInfo.boundingBoxMax = model.boundingBox.max;
   meshInfo.boundingBoxMin = model.boundingBox.min;
   meshInfo.materialID = this->get_material_id(cmdList, material);
   auto [emplacedIt, ok] = m_models.emplace(name, meshInfo);
   assert(ok);

   cmdList.copy_buffer(model.mesh.vertices.buffer(), m_combinedVertexBuffer.buffer(), 0, m_writtenVertexCount * sizeof(geometry::Vertex),
                       model.mesh.vertices.count() * sizeof(geometry::Vertex));

   m_writtenVertexCount += model.mesh.vertices.count();

   cmdList.copy_buffer(model.mesh.indices.buffer(), m_combinedIndexBuffer.buffer(), 0, m_writtenIndexCount * sizeof(u32),
                       model.mesh.indices.count() * sizeof(u32));

   m_writtenIndexCount += model.mesh.indices.count();

   return emplacedIt->second;
}

u32 BindlessScene::get_material_id(const graphics_api::CommandList& cmdList, const render_core::Material& material)
{
   if (material.materialTemplate == "pbr/simple.mt"_rc) {
      BindlessMaterialProps_AlbedoTex albedoTex;
      albedoTex.albedo = this->get_texture_id(std::get<TextureName>(material.values[0]));
      albedoTex.roughness = std::get<float>(material.values[1]);
      albedoTex.metalic = std::get<float>(material.values[2]);

      const auto outIndex = m_writtenMaterialProperty_AlbedoTex;

      cmdList.update_buffer(m_materialPropsAlbedoTex.buffer(),
                            m_writtenMaterialProperty_AlbedoTex * sizeof(BindlessMaterialProps_AlbedoTex),
                            sizeof(BindlessMaterialProps_AlbedoTex), &albedoTex);

      ++m_writtenMaterialProperty_AlbedoTex;

      return encode_material_id(1, outIndex);
   }
   if (material.materialTemplate == "pbr/normal_map.mt"_rc) {
      BindlessMaterialProps_AlbedoNormalTex albedoNormalTex;
      albedoNormalTex.albedo = this->get_texture_id(std::get<TextureName>(material.values[0]));
      albedoNormalTex.normal = this->get_texture_id(std::get<TextureName>(material.values[1]));
      albedoNormalTex.roughness = std::get<float>(material.values[2]);
      albedoNormalTex.metalic = std::get<float>(material.values[3]);

      const auto outIndex = m_writtenMaterialProperty_AlbedoNormalTex;

      cmdList.update_buffer(m_materialPropsAlbedoNormalTex.buffer(),
                            m_writtenMaterialProperty_AlbedoNormalTex * sizeof(BindlessMaterialProps_AlbedoNormalTex),
                            sizeof(BindlessMaterialProps_AlbedoNormalTex), &albedoNormalTex);

      ++m_writtenMaterialProperty_AlbedoNormalTex;

      return encode_material_id(2, outIndex);
   }
   return 0;
}

u32 BindlessScene::get_texture_id(const TextureName textureName)
{
   const auto it = m_textureIds.find(textureName);
   if (it != m_textureIds.end()) {
      return it->second;
   }

   m_shouldUpdatePSO = true;

   auto& texture = m_resourceManager.get(textureName);
   const auto textureId = m_sceneTextures.size();
   m_sceneTextures.emplace_back(&texture);

   m_textureIds.emplace(textureName, textureId);
   return textureId;
}

}// namespace triglav::renderer
