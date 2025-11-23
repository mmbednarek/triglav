#pragma once

#include "Camera.hpp"
#include "DebugLinesRenderer.hpp"
#include "OrthoCamera.hpp"

#include "triglav/Name.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/geometry/BVHTree.hpp"
#include "triglav/render_objects/Mesh.hpp"

#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace triglav::renderer {

using ObjectID = u32;
constexpr ObjectID UNSELECTED_OBJECT = ~0u;

class ModelRenderer;
class Renderer;

struct SceneObject
{
   MeshName model;
   String name;
   Transform3D transform;

   [[nodiscard]] Matrix4x4 model_matrix() const;
};

using SceneObjectUPtr = std::unique_ptr<SceneObject>;

struct SceneObjectRef
{
   SceneObject* object;
   geometry::BoundingBox bbox;
   ObjectID id;

   [[nodiscard]] const geometry::BoundingBox& bounding_box() const
   {
      return bbox;
   }
};

struct RayHit
{
   float distance;
   ObjectID id;
   const SceneObject* object;
};

class Scene
{
 public:
   TG_EVENT(OnObjectAddedToScene, ObjectID, const SceneObject&)
   TG_EVENT(OnObjectChangedTransform, ObjectID, const Transform3D&)
   TG_EVENT(OnObjectRemoved, ObjectID)
   TG_EVENT(OnViewportChange, const graphics_api::Resolution&)
   TG_EVENT(OnAddedBoundingBox, const geometry::BoundingBox&)
   TG_EVENT(OnShadowMapChanged, u32, const OrthoCamera&)
   TG_EVENT(OnViewUpdated, const Camera&)

   explicit Scene(resource::ResourceManager& resource_manager);

   void update(const graphics_api::Resolution& resolution);
   ObjectID add_object(SceneObject object);
   void set_transform(ObjectID object_id, const Transform3D& transform);
   void load_level(LevelName name);
   world::Level to_level() const;
   void set_camera(glm::vec3 position, glm::quat orientation);
   void update_shadow_maps();
   void send_view_changed();
   void remove_object(ObjectID object_id);

   [[nodiscard]] const Camera& camera() const;
   [[nodiscard]] Camera& camera();
   [[nodiscard]] const OrthoCamera& shadow_map_camera(u32 index) const;
   [[nodiscard]] u32 directional_shadow_map_count() const;
   const SceneObject& object(ObjectID id) const;

   [[nodiscard]] float yaw() const;
   [[nodiscard]] float pitch() const;

   void update_bvh();
   void update_orientation(float delta_yaw, float delta_pitch);
   void add_bounding_box(const geometry::BoundingBox& box) const;
   [[nodiscard]] const geometry::BVHTree<SceneObjectRef>& bvh() const;
   RayHit trace_ray(const geometry::Ray& ray) const;

   [[nodiscard]] std::map<ObjectID, SceneObjectUPtr>::const_iterator begin() const
   {
      return m_objects.cbegin();
   }

   [[nodiscard]] std::map<ObjectID, SceneObjectUPtr>::const_iterator end() const
   {
      return m_objects.cend();
   }

 private:
   resource::ResourceManager& m_resource_manager;
   float m_yaw{};
   float m_pitch{};
   Camera m_camera{};
   glm::quat m_directional_light_orientation{glm::vec3{geometry::g_pi * 0.1f, 0, geometry::g_pi * 1.5f}};
   std::array<OrthoCamera, 3> m_directional_shadow_map_cameras{};
   std::map<ObjectID, SceneObjectUPtr> m_objects{};
   geometry::BVHTree<SceneObjectRef> m_tree;
   ObjectID m_top_object_id = 0;
};

}// namespace triglav::renderer