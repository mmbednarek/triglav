#pragma once

#include "Camera.hpp"
#include "OrthoCamera.hpp"

#include "triglav/Event.hpp"
#include "triglav/Name.hpp"
#include "triglav/String.hpp"
#include "triglav/geometry/BVHTree.hpp"
#include "triglav/render_objects/Mesh.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace triglav::renderer {

constexpr world::EntityID UNSELECTED_OBJECT = ~0u;

class ModelRenderer;
class Renderer;

struct SceneObject
{
   MeshName model;
   String name;
   Transform3D transform;
   std::optional<ArmatureName> armature;

   [[nodiscard]] Matrix4x4 model_matrix() const;
};

using SceneObjectUPtr = std::unique_ptr<SceneObject>;

class Scene : public world::ISystem
{
   TG_DEFINE_LOG_CATEGORY(Scene)
 public:
   TG_TAG_CLASS(triglav::renderer::Scene)

   TG_EVENT(OnAddedBoundingBox, const geometry::BoundingBox&)
   TG_EVENT(OnShadowMapChanged, u32, const OrthoCamera&)
   TG_EVENT(OnTerrainUpdated, Vector2i, const std::vector<float>&, const std::vector<Vector4b>&)

   explicit Scene(resource::ResourceManager& resource_manager);

   void update_shadow_maps();

   void on_level_loaded(world::Level& level) override;
   void on_added_component(world::Level& level, Name component_name, world::ComponentID component_id,
                           std::span<const world::EntityID> entities) override;
   void on_removed_entities(world::Level& level, std::span<const world::EntityID> ids) override;
   void on_modified_component(world::Level& level, Name component_name, world::ComponentID component_id,
                              std::span<const world::EntityID> entities) override;
   Name system_name() override;

   [[nodiscard]] const OrthoCamera& shadow_map_camera(u32 index) const;
   [[nodiscard]] u32 directional_shadow_map_count() const;

   void add_bounding_box(const geometry::BoundingBox& box) const;
   std::vector<float>& terrain();
   std::vector<Vector4b>& terrain_blending();
   void publish_terrain_changes();

 private:
   resource::ResourceManager& m_resource_manager;
   glm::quat m_directional_light_orientation{glm::vec3{-0.3f, 0.0f, 1.62f}};
   std::array<OrthoCamera, 3> m_directional_shadow_map_cameras{};
   std::vector<float> m_terrain;
   std::vector<Vector4b> m_terrain_blending;
};

}// namespace triglav::renderer