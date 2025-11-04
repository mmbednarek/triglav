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
    m_resourceManager(resourceManager),
    m_scene(scene),
    m_device(device),
    m_sceneObjectStage(device, STAGING_BUFFER_ELEM_COUNT),
    m_sceneObjects(device, SCENE_ELEM_COUNT, gapi::BufferUsage::Indirect),
    m_combinedVertexBuffer(device, VERTEX_BUFFER_SIZE),
    m_combinedIndexBuffer(device, INDEX_BUFFER_SIZE),
    m_countBuffer(device, gapi::BufferUsage::Indirect),
    m_materialPropsAlbedoTex(device, 10),
    m_materialPropsAlbedoNormalTex(device, 10),
    m_materialPropsAllTex(device, 10),
    TG_CONNECT(scene, OnObjectAddedToScene, on_object_added_to_scene)
{
   TG_SET_DEBUG_NAME(m_sceneObjects.buffer(), "bindless_scene.scene_objects");
   TG_SET_DEBUG_NAME(m_sceneObjectStage.buffer(), "bindless_scene.scene_objects.staging");
   TG_SET_DEBUG_NAME(m_countBuffer.buffer(), "bindless_scene.count_buffer");
   TG_SET_DEBUG_NAME(m_combinedIndexBuffer.buffer(), "bindless_scene.combined_index_buffer");
   TG_SET_DEBUG_NAME(m_combinedVertexBuffer.buffer(), "bindless_scene.combined_vertex_buffer");
}

void BindlessScene::on_object_added_to_scene(ObjectID object_id, const SceneObject& object)
{
   m_pendingObjects.emplace_back(object_id, object);
}

void BindlessScene::on_update_scene(const gapi::CommandList& cmdList)
{
   if (m_pendingObjects.empty())
      return;

   {
      // TODO: Increase object buffer if necessary
      assert(m_pendingObjects.size() < STAGING_BUFFER_ELEM_COUNT);
      auto mapping{GAPI_CHECK(m_sceneObjectStage.buffer().map_memory())};
      for (const auto& [object_id, pending_object] : m_pendingObjects) {
         auto& meshInfo = this->get_mesh_info(cmdList, pending_object.model);
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
         object.transform = pending_object.model_matrix();
         object.normalTransform = glm::transpose(glm::inverse(glm::mat3(pending_object.model_matrix())));

         mapping.write_offset(&object, sizeof(BindlessSceneObject), m_writtenSceneObjectCount * sizeof(BindlessSceneObject));
         m_objectMapping[object_id] = m_writtenSceneObjectCount;
         ++m_writtenSceneObjectCount;
      }
   }

   *m_countBuffer = static_cast<u32>(m_writtenSceneObjectCount);

   cmdList.copy_buffer(m_sceneObjectStage.buffer(), m_sceneObjects.buffer(), 0,
                       static_cast<u32>(m_writtenObjectCount * sizeof(BindlessSceneObject)),
                       static_cast<u32>(m_pendingObjects.size() * sizeof(BindlessSceneObject)));

   m_pendingObjects.clear();
}

void BindlessScene::write_object_to_buffer()
{
   const auto copyObjectsCmdList = GAPI_CHECK(m_device.create_command_list(graphics_api::WorkType::Transfer));
   GAPI_CHECK_STATUS(copyObjectsCmdList.begin());

   this->on_update_scene(copyObjectsCmdList);

   GAPI_CHECK_STATUS(copyObjectsCmdList.finish());

   const auto fence = GAPI_CHECK(m_device.create_fence());
   fence.await();

   const graphics_api::SemaphoreArray empty;
   GAPI_CHECK_STATUS(m_device.submit_command_list(copyObjectsCmdList, empty, empty, &fence, graphics_api::WorkType::Transfer));

   fence.await();
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
      return m_materialPropsAlbedoTex.buffer();
   case 1:
      return m_materialPropsAlbedoNormalTex.buffer();
   default:
      return m_materialPropsAllTex.buffer();
   }
}

const gapi::Buffer& BindlessScene::count_buffer() const
{
   return m_countBuffer.buffer();
}

u32 BindlessScene::scene_object_count() const
{
   return static_cast<u32>(m_writtenSceneObjectCount);
}

Scene& BindlessScene::scene() const
{
   return m_scene;
}

std::vector<const graphics_api::Texture*>& BindlessScene::scene_textures()
{
   return m_sceneTextures;
}

std::vector<render_core::TextureRef>& BindlessScene::scene_texture_refs()
{
   return m_sceneTextureRefs;
}

BindlessMeshInfo& BindlessScene::get_mesh_info(const gapi::CommandList& cmdList, const MeshName name)
{
   if (const auto it = m_models.find(name); it != m_models.end()) {
      return it->second;
   }

   auto& model = m_resourceManager.get(name);

   assert(!model.range.empty());
   auto material = m_resourceManager.get(model.range[0].materialName);

   BindlessMeshInfo meshInfo;
   meshInfo.indexCount = static_cast<u32>(model.mesh.indices.count());
   meshInfo.indexOffset = static_cast<u32>(m_writtenIndexCount);
   meshInfo.vertexOffset = static_cast<u32>(m_writtenVertexCount);
   meshInfo.boundingBoxMax = model.boundingBox.max;
   meshInfo.boundingBoxMin = model.boundingBox.min;
   meshInfo.materialID = this->get_material_id(cmdList, material);
   auto [emplacedIt, ok] = m_models.emplace(name, meshInfo);
   assert(ok);

   cmdList.copy_buffer(model.mesh.vertices.buffer(), m_combinedVertexBuffer.buffer(), 0,
                       static_cast<u32>(m_writtenVertexCount * sizeof(geometry::Vertex)),
                       static_cast<u32>(model.mesh.vertices.count() * sizeof(geometry::Vertex)));

   m_writtenVertexCount += model.mesh.vertices.count();

   cmdList.copy_buffer(model.mesh.indices.buffer(), m_combinedIndexBuffer.buffer(), 0, static_cast<u32>(m_writtenIndexCount * sizeof(u32)),
                       static_cast<u32>(model.mesh.indices.count() * sizeof(u32)));

   m_writtenIndexCount += model.mesh.indices.count();

   return emplacedIt->second;
}

u32 BindlessScene::get_material_id(const graphics_api::CommandList& cmdList, const render_objects::Material& material)
{
   switch (material.materialTemplate) {
   case render_objects::MaterialTemplate::Basic: {
      const auto& props = std::get<render_objects::MTProperties_Basic>(material.properties);

      Properties_MT0 albedoTex;
      albedoTex.albedoTextureID = this->get_texture_id(props.albedo);
      albedoTex.roughness = props.roughness;
      albedoTex.metallic = props.metallic;

      const auto outIndex = static_cast<u32>(m_writtenMaterialProperty_AlbedoTex);

      cmdList.update_buffer(m_materialPropsAlbedoTex.buffer(),
                            static_cast<u32>(m_writtenMaterialProperty_AlbedoTex * sizeof(Properties_MT0)), sizeof(Properties_MT0),
                            &albedoTex);

      ++m_writtenMaterialProperty_AlbedoTex;

      return encode_material_id(0, outIndex);
   }
   case render_objects::MaterialTemplate::NormalMap: {
      const auto& props = std::get<render_objects::MTProperties_NormalMap>(material.properties);

      Properties_MT1 albedoNormalTex;
      albedoNormalTex.albedoTextureID = this->get_texture_id(props.albedo);
      albedoNormalTex.normalTextureID = this->get_texture_id(props.normal);
      albedoNormalTex.roughness = props.roughness;
      albedoNormalTex.metallic = props.metallic;

      const auto outIndex = static_cast<u32>(m_writtenMaterialProperty_AlbedoNormalTex);

      cmdList.update_buffer(m_materialPropsAlbedoNormalTex.buffer(),
                            static_cast<u32>(m_writtenMaterialProperty_AlbedoNormalTex * sizeof(Properties_MT1)), sizeof(Properties_MT1),
                            &albedoNormalTex);

      ++m_writtenMaterialProperty_AlbedoNormalTex;

      return encode_material_id(1, outIndex);
   }
   case render_objects::MaterialTemplate::FullPBR: {
      const auto& props = std::get<render_objects::MTProperties_FullPBR>(material.properties);

      Properties_MT2 allTex;
      allTex.albedoTextureID = this->get_texture_id(props.texture);
      allTex.normalTextureID = this->get_texture_id(props.normal);
      allTex.roughnessTextureID = this->get_texture_id(props.roughness);
      allTex.metallicTextureID = this->get_texture_id(props.metallic);

      const auto outIndex = static_cast<u32>(m_writtenMaterialProperty_AllTex);

      cmdList.update_buffer(m_materialPropsAllTex.buffer(), static_cast<u32>(m_writtenMaterialProperty_AllTex * sizeof(Properties_MT2)),
                            sizeof(Properties_MT2), &allTex);

      ++m_writtenMaterialProperty_AllTex;

      return encode_material_id(2, outIndex);
   }
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
   const auto textureId = static_cast<u32>(m_sceneTextures.size());
   m_sceneTextures.emplace_back(&texture);
   m_sceneTextureRefs.emplace_back(textureName);

   m_textureIds.emplace(textureName, textureId);
   return textureId;
}

}// namespace triglav::renderer
