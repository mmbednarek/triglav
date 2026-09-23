#include "Scene.hpp"

#include "Renderer.hpp"

#include "triglav/engine/Engine.hpp"
#include "triglav/world/Level.hpp"

#include <cmath>
#include <glm/gtc/quaternion.hpp>

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

const OrthoCamera& Scene::shadow_map_camera(const u32 index) const
{
   return m_directional_shadow_map_cameras[index];
}

u32 Scene::directional_shadow_map_count() const
{
   return static_cast<u32>(m_directional_shadow_map_cameras.size());
}

void Scene::add_bounding_box(const geometry::BoundingBox& box) const
{
   event_OnAddedBoundingBox.publish(box);
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

void Scene::update_shadow_maps()
{
   Camera cam;// TODO: Get from ViewContext
   auto sm_props1 = cam.calculate_shadow_map(m_directional_light_orientation, 32.0f, 120.0f);
   m_directional_shadow_map_cameras[0] = OrthoCamera::from_properties(sm_props1);
   event_OnShadowMapChanged.publish(0, m_directional_shadow_map_cameras[0]);

   auto sm_props2 = cam.calculate_shadow_map(m_directional_light_orientation, 72.0f, 192.0f);
   m_directional_shadow_map_cameras[1] = OrthoCamera::from_properties(sm_props2);
   event_OnShadowMapChanged.publish(1, m_directional_shadow_map_cameras[1]);

   auto sm_props3 = cam.calculate_shadow_map(m_directional_light_orientation, 180.0f, 256.0f);
   m_directional_shadow_map_cameras[2] = OrthoCamera::from_properties(sm_props3);
   event_OnShadowMapChanged.publish(2, m_directional_shadow_map_cameras[2]);
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