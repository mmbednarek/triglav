#include "LevelBVH.hpp"

#include "triglav/geometry/BVHUtil.hpp"

#include "MeshBVHStorage.hpp"
#include "Physics.hpp"
#include "triglav/engine/Engine.hpp"

namespace triglav::physics {

namespace {

constexpr float EXTENSION_MULTIPLIER = 1.2f;

struct LevelBVHBaseTraits
{
   using Node = LevelBVH::Node;
   struct UserData
   {
      LevelBVH::NodeMappingContainer* node_mapping;
   };

   static Node* move_payload(UserData* user_data, Node* old_leaf)
   {
      auto* new_leaf = new Node{
         .parent = old_leaf,
         .bounding_box = old_leaf->bounding_box,
         .is_leaf_node = true,
         .leaf = old_leaf->leaf,
      };

      user_data->node_mapping->operator[](new_leaf->leaf.entity_id) = new_leaf;

      old_leaf->is_leaf_node = false;
      old_leaf->internal.left = nullptr;
      old_leaf->internal.right = nullptr;

      return new_leaf;
   }

   static float calculate_insert_cost(UserData* /*user_data*/, const Node* parent, const Node* new_child)
   {
      return parent->bounding_box.extension_cost(new_child->bounding_box);
   }

   static void recalculate_bounds(UserData* /*user_data*/, Node* parent)
   {
      const auto new_bb = parent->internal.left->bounding_box.merge(parent->internal.right->bounding_box);
      parent->bounding_box = new_bb;

      parent->internal.max_surface_area = EXTENSION_MULTIPLIER * new_bb.half_surface_area();
      parent->internal.left->parent = parent;
      parent->internal.right->parent = parent;
   }
};

using LevelBVHUtil = geometry::BVHUtil<LevelBVHBaseTraits>;

struct LevelBVHRayTraversalTraits
{
   using BaseTraits = LevelBVHBaseTraits;
   using Node = BaseTraits::Node;
   using Result = LevelBVH::RayHit;
   using Traversary = geometry::Ray;
   using IntersectionResult = geometry::BoundingBox::RayHit;
   struct UserData
   {
      const BVHProvider* mesh_provider;
   };

   static Result test_leaf(const UserData* user_data, const Node* node, const geometry::Ray& ray, float& best_score)
   {
      assert(node->is_leaf_node);
      auto& bottom_level = user_data->mesh_provider->get_bottom_level(node->leaf.entity_id);
      const auto bl_ray = ray.to_local_space(node->leaf.inv_transform);
      const auto bl_hit = bottom_level.ray_cast(bl_ray);
      if (bl_hit.distance > best_score)
         return Result::miss();

      best_score = bl_hit.distance;
      return Result{bl_hit.distance, node->leaf.entity_id};
   }

   static IntersectionResult test_intersection(const UserData* /*user_data*/, const Node* node, const geometry::Ray& ray)
   {
      return IntersectionResult{node->bounding_box.intersect(ray)};
   }

   static float test_hit_score(const UserData* /*user_data*/, const IntersectionResult& inter_result, const Node* /*node*/,
                               const Traversary& /*traversary*/)
   {
      assert(inter_result.has_hit());
      return inter_result.min_distance;
   }
};

using LevelBVHRayTraverser = geometry::BVHTraverser<LevelBVHRayTraversalTraits>;

struct LevelBVHSweepTraversalTraits
{
   using BaseTraits = LevelBVHBaseTraits;
   using Node = BaseTraits::Node;
   using Result = LevelBVH::SweepHit;
   struct Traversary
   {
      const geometry::BoundingBox& bounding_box;
      Vector3 movement;
   };
   struct UserData
   {
      const BVHProvider* mesh_provider;
   };

   struct IntersectionResult
   {
      bool does_intersect;

      [[nodiscard]] bool has_hit() const
      {
         return does_intersect;
      }
   };

   static Result test_leaf(const UserData* user_data, const Node* node, const Traversary& traversary, float& best_score)
   {
      assert(node != nullptr && node->is_leaf_node);
      const auto sweep_result = node->bounding_box.intersect_swept(traversary.bounding_box, traversary.movement);
      if (!sweep_result.has_hit()) {
         return Result::miss();
      }

      auto& bottom_level = user_data->mesh_provider->get_bottom_level(node->leaf.entity_id);
      const auto ls_bb = traversary.bounding_box.transform(node->leaf.inv_transform);
      const auto ls_movement = node->leaf.inv_transform.shrink() * traversary.movement;
      const auto result = bottom_level.sweep_cast(ls_bb, ls_movement);
      if (result.primitive_id == ~0u) {
         return Result::miss();
      }
      if (result.time >= best_score) {
         return Result::miss();
      }
      best_score = result.time;

      return Result{
         .time = result.time,
         .normal = (node->leaf.inv_transform.shrink().transpose() * result.normal).normalize(),
         .entity_id = node->leaf.entity_id,
      };
   }

   static IntersectionResult test_intersection(const UserData* /*user_data*/, const Node* node, const Traversary& traversary)
   {
      return IntersectionResult{node->bounding_box.does_intersect_aabb(traversary.bounding_box.extend_by_movement(traversary.movement))};
   }

   static float test_hit_score(const UserData* /*user_data*/, const IntersectionResult& /*inter_result*/, const Node* node,
                               const Traversary& traversary)
   {
      return node->bounding_box.intersect_swept(traversary.bounding_box, traversary.movement).entry_time;
   }
};

using LevelBVHSweepTraverser = geometry::BVHTraverser<LevelBVHSweepTraversalTraits>;

void update_parent_bounds(LevelBVH::Node* node)
{
   while (node != nullptr) {
      if (node->internal.left != nullptr && node->internal.right != nullptr) {
         node->bounding_box = node->internal.left->bounding_box.merge(node->internal.right->bounding_box);
      }
      node = node->parent;
   }
}

[[nodiscard]] geometry::BoundingBox calculate_bounding_box(const std::span<LevelBVH::Node*> data)
{
   geometry::BoundingBox result{
      .min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
      .max = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()},
   };
   for (const auto& pd : data) {
      const auto bb = pd->bounding_box;
      result.min = {std::min(result.min.x, bb.min.x), std::min(result.min.y, bb.min.y), std::min(result.min.z, bb.min.z)};
      result.max = {std::max(result.max.x, bb.max.x), std::max(result.max.y, bb.max.y), std::max(result.max.z, bb.max.z)};
   }
   return result;
}

}// namespace

LevelBVH::LevelBVH(world::Level& level, BVHProvider& provider) :
    m_level(level),
    m_mesh_provider(provider)
{
}

LevelBVH::~LevelBVH()
{
   this->clear();
}

LevelBVH::LevelBVH(LevelBVH&& other) noexcept :
    m_level(other.m_level),
    m_mesh_provider(other.m_mesh_provider),
    m_root(std::exchange(other.m_root, nullptr)),
    m_leave_mapping(std::move(other.m_leave_mapping))
{
}

LevelBVH& LevelBVH::operator=(LevelBVH&& other) noexcept
{
   this->clear();

   m_root = std::exchange(other.m_root, nullptr);
   m_leave_mapping = std::move(other.m_leave_mapping);
   return *this;
}

namespace {

LevelBVH::Node* leaf_from_entity_id(const world::Level& level, const world::EntityID entity_id, const world::Mesh* mesh_comp = nullptr,
                                    const Movement* movement_comp = nullptr)
{
   if (mesh_comp == nullptr) {
      mesh_comp = level.component_opt<world::Mesh>(entity_id);
      assert(mesh_comp != nullptr);
   }
   if (movement_comp == nullptr) {
      movement_comp = level.component_opt<Movement>(entity_id);
   }
   const auto& mesh = engine::resource_manager().get(mesh_comp->name);
   const auto* transform_ptr = level.component_opt<Transform3D>(entity_id);
   Transform3D transform = transform_ptr == nullptr ? Transform3D::identity() : *transform_ptr;
   const auto transform_matrix = transform.to_matrix();
   auto bb = mesh.bounding_box.transform(transform_matrix);
   if (movement_comp != nullptr) {
      bb = bb.extend_by_movement(movement_comp->motion);
   }

   return new LevelBVH::Node{
      .parent = nullptr,
      .bounding_box = bb,
      .is_leaf_node = true,
      .leaf =
         {
            .entity_id = entity_id,
            .inv_transform = transform_matrix.inverse(),
         },
   };
}

struct EntityData
{
   geometry::BoundingBox bounding_box;
   Matrix4x4 inv_transform;
};

EntityData get_entity_data(const world::Level& level, const world::EntityID entity_id)
{
   const auto* mesh_comp = level.component_opt<world::Mesh>(entity_id);
   if (mesh_comp == nullptr) {
      return {};
   }
   const auto* transform_ptr = level.component_opt<Transform3D>(entity_id);
   Transform3D transform = transform_ptr == nullptr ? Transform3D::identity() : *transform_ptr;
   Matrix4x4 matrix = transform.to_matrix();

   const auto& mesh = engine::resource_manager().get(mesh_comp->name);
   return {mesh.bounding_box.transform(matrix), matrix.inverse()};
}

LevelBVH::Node* build_bvh(std::span<LevelBVH::Node*> nodes, LevelBVH::Node* parent)
{
   if (nodes.empty())
      return nullptr;
   if (nodes.size() == 1) {
      return nodes[0];
   }

   const auto bb = calculate_bounding_box(nodes);
   auto axis = bb.major_axis();

   std::ranges::nth_element(nodes, nodes.begin() + (nodes.size() / 2), [axis](const LevelBVH::Node* left, const LevelBVH::Node* right) {
      return left->bounding_box.centroid()[axis] > right->bounding_box.centroid()[axis];
   });

   const auto mid = nodes.size() / 2;

   auto* node = new LevelBVH::Node{
      .parent = parent,
      .bounding_box = bb,
      .is_leaf_node = false,
      .internal =
         {
            .max_surface_area = EXTENSION_MULTIPLIER * bb.half_surface_area(),
         },
   };
   node->internal.left = build_bvh(nodes.subspan(0, mid), node);
   node->internal.right = build_bvh(nodes.subspan(mid, nodes.size() - mid), node);

   return node;
}

}// namespace

void LevelBVH::add(const world::EntityID entity_id)
{
   auto* node = leaf_from_entity_id(m_level, entity_id);

   m_leave_mapping[entity_id] = node;

   if (m_root == nullptr) {
      m_root = node;
   } else {
      LevelBVHUtil::UserData user_data{
         .node_mapping = &m_leave_mapping,
      };
      LevelBVHUtil::insert_node(&user_data, m_root, node);
   }

   assert(node->is_leaf());
}

void LevelBVH::remove(const world::EntityID entity_id)
{
   const auto leaf_it = m_leave_mapping.find(entity_id);
   if (leaf_it == m_leave_mapping.end()) {
      return;
   }

   auto* leaf = leaf_it->second;
   m_leave_mapping.erase(leaf_it);

   auto* parent = leaf->parent;
   if (parent == nullptr) {
      delete leaf;
      m_root = nullptr;
      return;
   }

   auto* sibling = parent->internal.left == leaf ? parent->internal.right : parent->internal.left;
   auto* grandparent = parent->parent;

   sibling->parent = grandparent;
   if (grandparent == nullptr) {
      m_root = sibling;
   } else if (grandparent->internal.left == parent) {
      grandparent->internal.left = sibling;
   } else {
      grandparent->internal.right = sibling;
   }

   parent->internal.left = nullptr;
   parent->internal.right = nullptr;
   delete leaf;
   delete parent;

   update_parent_bounds(grandparent);
}

void LevelBVH::update(const world::EntityID entity_id)
{
   const auto leaf_it = m_leave_mapping.find(entity_id);
   if (leaf_it == m_leave_mapping.end()) {
      return;
   }
   auto data = get_entity_data(m_level, entity_id);

   bool needs_to_update_parents = false;

   auto* parent = leaf_it->second->parent;
   if (parent != nullptr) {
      assert(leaf_it->second->is_leaf());
      const auto old_bb = leaf_it->second->bounding_box;
      const auto new_bb = data.bounding_box;
      if (old_bb != new_bb) {
         const auto* sibling_node = parent->internal.left == leaf_it->second ? parent->internal.right : parent->internal.left;

         const auto new_parent_bb = new_bb.merge(sibling_node->bounding_box);
         needs_to_update_parents = true;

         const auto new_parent_surface_area = new_parent_bb.half_surface_area();
         if (new_parent_surface_area > parent->internal.max_surface_area) {
            this->remove(entity_id);
            this->add(entity_id);
            return;
         }
      }
   }

   auto* leaf = leaf_it->second;
   leaf->bounding_box = data.bounding_box;
   leaf->leaf = {
      .entity_id = entity_id,
      .inv_transform = data.inv_transform,
   };

   if (needs_to_update_parents) {
      update_parent_bounds(leaf->parent);
   }
}

void LevelBVH::build_static()
{
   this->clear();

   std::vector<Node*> nodes;
   nodes.reserve(m_level.component_count<world::Mesh>());

   for (const auto& [entity_id, mesh_comp] : m_level.all<world::Mesh>()) {
      if (m_level.has_component<Movement>(entity_id))
         continue;
      auto* node = leaf_from_entity_id(m_level, entity_id, &mesh_comp);
      m_leave_mapping[entity_id] = node;
      nodes.emplace_back(node);
   }

   m_root = build_bvh(nodes, nullptr);
}

void LevelBVH::build_dynamic()
{
   this->clear();

   std::vector<Node*> nodes;
   nodes.reserve(m_level.component_count<Movement>());

   for (const auto& [entity_id, movement_comp] : m_level.all<Movement>()) {
      const auto* mesh_comp = m_level.component_opt<world::Mesh>(entity_id);
      if (mesh_comp == nullptr)
         continue;
      auto* node = leaf_from_entity_id(m_level, entity_id, mesh_comp, &movement_comp);
      m_leave_mapping[entity_id] = node;
      nodes.emplace_back(node);
   }

   m_root = build_bvh(nodes, nullptr);
}

void LevelBVH::clear()
{
   if (m_root == nullptr) {
      return;
   }
   LevelBVHUtil::destroy(m_root);
   m_root = nullptr;
}

LevelBVH::RayHit LevelBVH::ray_cast(const geometry::Ray& ray) const
{
   if (m_root == nullptr) {
      return RayHit::miss();
   }

   geometry::Ray mut_ray = ray;
   LevelBVHRayTraverser::UserData user_data{
      .mesh_provider = &m_mesh_provider,
   };
   return LevelBVHRayTraverser::traverse(&user_data, m_root, mut_ray, mut_ray.distance);
}

LevelBVH::SweepHit LevelBVH::sweep_cast(const geometry::BoundingBox& bb, const Vector3& movement) const
{
   float hit_time = INF;
   LevelBVHSweepTraverser::UserData user_data{
      .mesh_provider = &m_mesh_provider,
   };
   return LevelBVHSweepTraverser::traverse(&user_data, m_root, {bb, movement}, hit_time);
}

bool LevelBVH::contains(const world::EntityID entity_id) const
{
   return m_leave_mapping.contains(entity_id);
}

geometry::BoundingBox LevelBVH::bounding_box(const world::EntityID entity_id) const
{
   const auto it = m_leave_mapping.find(entity_id);
   if (it == m_leave_mapping.end()) {
      return {};
   }

   return it->second->bounding_box;
}

namespace {

void find_overlap_recursive(std::vector<std::pair<world::EntityID, world::EntityID>>& out_overlaps, LevelBVH::Node* left,
                            LevelBVH::Node* right)
{
   if (!left->bounding_box.does_intersect_aabb(right->bounding_box))
      return;

   const auto ll = left->is_leaf();
   const auto lr = right->is_leaf();

   if (ll && lr) {
      out_overlaps.emplace_back(left->leaf.entity_id, right->leaf.entity_id);
      return;
   }
   if (!ll) {
      find_overlap_recursive(out_overlaps, left->internal.left, right);
      find_overlap_recursive(out_overlaps, left->internal.right, right);
   }
   if (!lr) {
      find_overlap_recursive(out_overlaps, left, right->internal.left);
      find_overlap_recursive(out_overlaps, left, right->internal.right);
   }
}

}// namespace

void LevelBVH::find_overlaps(std::vector<std::pair<world::EntityID, world::EntityID>>& out_overlaps) const
{
   if (m_root == nullptr)
      return;
   if (m_root->is_leaf())
      return;

   find_overlap_recursive(out_overlaps, m_root->internal.left, m_root->internal.right);
}

}// namespace triglav::physics
