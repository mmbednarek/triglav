#include "BindlessScene.hpp"

namespace triglav::renderer {

namespace gapi = graphics_api;

constexpr auto STAGING_BUFFER_ELEM_COUNT = 64;
constexpr auto SCENE_ELEM_COUNT = 128;
constexpr auto VERTEX_BUFFER_SIZE = 256000;
constexpr auto INDEX_BUFFER_SIZE = 256000;

BindlessScene::BindlessScene(gapi::Device& device, resource::ResourceManager& resourceManager, Scene& scene) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_scene(scene),
    m_sceneObjectStage(device, STAGING_BUFFER_ELEM_COUNT),
    m_sceneObjects(device, SCENE_ELEM_COUNT, gapi::BufferUsage::Indirect),
    m_combinedVertexBuffer(device, VERTEX_BUFFER_SIZE),
    m_combinedIndexBuffer(device, INDEX_BUFFER_SIZE),
    m_countBuffer(device, gapi::BufferUsage::Indirect),
    m_solidColorProps(device, 1),
    TG_CONNECT(scene, OnObjectAddedToScene, on_object_added_to_scene)
{
   SolidColorProperties properties{};
   properties.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
   properties.metalic = 0.5;
   properties.roughness = 0.5;
   m_solidColorProps.write(&properties, 1);
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

         mapping.write_offset(&object, sizeof(BindlessSceneObject), m_writtenSceneObjectCount * sizeof(BindlessSceneObject));
         ++m_writtenSceneObjectCount;
      }
   }

   *m_countBuffer = m_writtenSceneObjectCount;

   cmdList.copy_buffer(m_sceneObjectStage.buffer(), m_sceneObjects.buffer(), 0, m_writtenObjectCount * sizeof(BindlessSceneObject),
                       m_pendingObjects.size() * sizeof(BindlessSceneObject));
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
   return m_solidColorProps.buffer();
}

const gapi::Buffer& BindlessScene::count_buffer() const
{
   return m_countBuffer.buffer();
}

Scene& BindlessScene::scene() const
{
   return m_scene;
}

BindlessMeshInfo& BindlessScene::get_mesh_info(const gapi::CommandList& cmdList, const ModelName name)
{
   if (const auto it = m_models.find(name); it != m_models.end()) {
      return it->second;
   }

   auto& model = m_resourceManager.get(name);

   BindlessMeshInfo meshInfo;
   meshInfo.indexCount = model.mesh.indices.count();
   meshInfo.indexOffset = m_writtenIndexCount;
   meshInfo.vertexOffset = m_writtenVertexCount;
   meshInfo.boundingBoxMax = model.boundingBox.max;
   meshInfo.boundingBoxMin = model.boundingBox.min;
   meshInfo.materialID = 0;// TODO: Add material ID
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

}// namespace triglav::renderer
