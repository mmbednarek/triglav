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
   std::ranges::fill(m_terrain_blending, 0);
}

void Scene::update(const graphics_api::Resolution& resolution)
{
   const auto [width, height] = resolution;
   m_camera.set_viewport_size(static_cast<float>(width), static_cast<float>(height));

   event_OnViewportChange.publish(resolution);
   this->send_view_changed();
}

void Scene::add_object(SceneObject object, world::EntityID entity_id)
{
   const auto& [it, ok] = m_objects.emplace(entity_id, std::make_unique<SceneObject>(std::move(object)));
   assert(ok);
   event_OnObjectAddedToScene.publish(it->first, *it->second);
   this->update_bvh();
}

void Scene::set_transform(const world::EntityID entity_id, const Transform3D& transform)
{
   m_objects[entity_id]->transform = transform;
   this->update_bvh();

   event_OnObjectChangedTransform.publish(entity_id, transform);
}

void Scene::set_camera(const glm::vec3 position, const glm::quat orientation)
{
   m_camera.set_position(position);
   m_camera.set_orientation(orientation);
   event_OnViewUpdated.publish(m_camera);
}

const Camera& Scene::camera() const
{
   return m_camera;
}

Camera& Scene::camera()
{
   return m_camera;
}

const OrthoCamera& Scene::shadow_map_camera(const u32 index) const
{
   return m_directional_shadow_map_cameras[index];
}

u32 Scene::directional_shadow_map_count() const
{
   return static_cast<u32>(m_directional_shadow_map_cameras.size());
}

const SceneObject& Scene::object(const world::EntityID id) const
{
   return *m_objects.at(id);
}

float Scene::yaw() const
{
   return m_yaw;
}

float Scene::pitch() const
{
   return m_pitch;
}

void Scene::update_bvh()
{
   std::vector<SceneObjectRef> objects{m_objects.size()};

   auto obj_it = objects.begin();
   for (const auto& [id, scene_object] : m_objects) {
      const auto& mesh = m_resource_manager.get(scene_object->model);
      *(obj_it++) = SceneObjectRef{
         .object = scene_object.get(),
         .bbox = mesh.bounding_box.transform(scene_object->transform.to_matrix()),
         .id = id,
      };
   }

   m_tree.build(objects);
}

void Scene::update_orientation(const float delta_yaw, const float delta_pitch)
{
   m_yaw += delta_yaw;
   while (m_yaw < 0) {
      m_yaw += static_cast<float>(2 * g_pi);
   }
   while (m_yaw >= static_cast<float>(2 * g_pi)) {
      m_yaw -= static_cast<float>(2 * g_pi);
   }

   m_pitch += delta_pitch;
   m_pitch = std::clamp(m_pitch, -static_cast<float>(g_pi) / 2.0f + 0.01f, static_cast<float>(g_pi) / 2.0f - 0.01f);

   this->camera().set_orientation(glm::quat{glm::vec3{m_pitch, 0.0f, m_yaw}});
   this->send_view_changed();
}

void Scene::add_bounding_box(const geometry::BoundingBox& box) const
{
   event_OnAddedBoundingBox.publish(box);
}

const geometry::BVHTree<SceneObjectRef>& Scene::bvh() const
{
   return m_tree;
}

RayHit Scene::trace_ray(const geometry::Ray& ray) const
{
   const auto hit = this->bvh().traverse(ray);
   if (hit.payload == nullptr)
      return {INFINITY, ~0u, nullptr};
   return {hit.distance, hit.payload->id, hit.payload->object};
}

std::vector<float>& Scene::terrain()
{
   return m_terrain;
}

std::vector<u8>& Scene::terrain_blending()
{
   return m_terrain_blending;
}

void Scene::publish_terrain_changes()
{
   event_OnTerrainUpdated.publish(Vector2i{1024, 1024}, m_terrain, m_terrain_blending);
}

void Scene::update_shadow_maps()
{
   auto sm_props1 = this->camera().calculate_shadow_map(m_directional_light_orientation, 32.0f, 120.0f);
   m_directional_shadow_map_cameras[0] = OrthoCamera::from_properties(sm_props1);
   event_OnShadowMapChanged.publish(0, m_directional_shadow_map_cameras[0]);

   auto sm_props2 = this->camera().calculate_shadow_map(m_directional_light_orientation, 72.0f, 192.0f);
   m_directional_shadow_map_cameras[1] = OrthoCamera::from_properties(sm_props2);
   event_OnShadowMapChanged.publish(1, m_directional_shadow_map_cameras[1]);

   auto sm_props3 = this->camera().calculate_shadow_map(m_directional_light_orientation, 180.0f, 256.0f);
   m_directional_shadow_map_cameras[2] = OrthoCamera::from_properties(sm_props3);
   event_OnShadowMapChanged.publish(2, m_directional_shadow_map_cameras[2]);
}

void Scene::send_view_changed() const
{
   event_OnViewUpdated.publish(m_camera);
}

void Scene::remove_object(const world::EntityID entity_id)
{
   event_OnObjectRemoved.publish(entity_id);
   m_objects.erase(entity_id);
   this->update_bvh();
}

void Scene::set_object_name(const world::EntityID id, const StringView name) const
{
   const auto& obj = m_objects.at(id);
   obj->name = name;
   event_OnObjectChangedName.publish(id, name);
}

void Scene::on_level_loaded(world::Level& level)
{
   std::vector<world::EntityID> entities;
   for (const auto& [entity_id, com] : level.all<world::Mesh>()) {
      entities.emplace_back(entity_id);
   }

   this->on_added_component(level, "triglav::world::Mesh"_name, 0, entities);
}

void Scene::on_removed_entities(world::Level& /*level*/, const std::span<const world::EntityID> ids)
{
   for (const auto entity_id : ids) {
      event_OnObjectRemoved.publish(entity_id);
      m_objects.erase(entity_id);
   }

   this->update_bvh();
}

void Scene::on_added_component(world::Level& level, Name component_name, world::ComponentID /*component_id*/,
                               std::span<const world::EntityID> entities)
{
   if (component_name != "triglav::world::Mesh"_name)
      return;

   for (const auto entity_id : entities) {
      const auto [label, transform, mesh] = level.components<world::EntityLabel, Transform3D, world::Mesh>(entity_id);

      SceneObject obj{
         .model = mesh.name,
         .name = label.label.c_str(),
         .transform = transform,
         .armature = std::nullopt,
      };

      if (const auto* arm = level.component_opt<world::Armature>(entity_id); arm != nullptr) {
         obj.armature = arm->name;
      }
      this->add_object(obj, entity_id);
   }
}

void Scene::on_modified_component(world::Level& level, const Name component_name, world::ComponentID /*component_id*/,
                                  std::span<const world::EntityID> entities)
{
   if (component_name == "triglav::Transform3D"_name) {
      for (const auto entity_id : entities) {
         const auto& transform = level.component<Transform3D>(entity_id);
         m_objects[entity_id]->transform = transform;
         event_OnObjectChangedTransform.publish(entity_id, transform);
      }
      this->update_bvh();
   } else if (component_name == "triglav::world::EntityLabel"_name) {
      for (const auto entity_id : entities) {
         const auto& label = level.component<world::EntityLabel>(entity_id);

         const auto& obj = m_objects.at(entity_id);
         obj->name = {label.label.data(), label.label.size()};
         event_OnObjectChangedName.publish(entity_id, obj->name.view());
      }
   }
}

Name Scene::system_name()
{
   return TAG;
}

}// namespace triglav::renderer