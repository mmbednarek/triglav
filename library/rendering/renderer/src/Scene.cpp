#include "Scene.hpp"

#include "Renderer.hpp"

#include "triglav/engine/Engine.hpp"
#include "triglav/world/Level.hpp"

using triglav::ResourceType;

namespace triglav::renderer {

using namespace name_literals;

engine::SystemRegisterer SCENE_REGISTERER{{
   .constructor = [](world::Level& /*level*/) -> std::unique_ptr<world::ISystem> {
      return std::make_unique<Scene>(engine::the().resource_manager());
   },
   .components =
      std::vector<Name>{
         "triglav::Transform3D"_name,
         "triglav::world::Mesh"_name,
      },
}};

Matrix4x4 SceneObject::model_matrix() const
{
   return this->transform.to_matrix();
}

Scene::Scene(resource::ResourceManager& resource_manager) :
    m_resource_manager(resource_manager)
{
   m_terrain.resize(1024 * 1024);
   std::ranges::fill(m_terrain, 0.0f);

   m_terrain_blending.resize(1024 * 1024);
   std::ranges::fill(m_terrain_blending, Vector4b{1, 0, 0, 0});
}

std::vector<float>& Scene::terrain()
{
   return m_terrain;
}

std::vector<Vector4b>& Scene::terrain_blending()
{
   return m_terrain_blending;
}

void Scene::publish_terrain_changes()
{
   event_OnTerrainUpdated.publish(Vector2i{1024, 1024}, m_terrain, m_terrain_blending);
}

Name Scene::system_name()
{
   return TAG;
}

void Scene::on_level_loaded(world::Level& /*level*/) {}

void Scene::on_added_component(world::Level& /*level*/, Name /*component_name*/, world::ComponentID /*component_id*/,
                               std::span<const world::EntityID> /*entities*/)
{
}

void Scene::on_removed_entities(world::Level& /*level*/, const std::span<const world::EntityID> /*ids*/) {}

void Scene::on_modified_component(world::Level& /*level*/, const Name /*component_name*/, world::ComponentID /*component_id*/,
                                  std::span<const world::EntityID> /*entities*/)
{
}

}// namespace triglav::renderer