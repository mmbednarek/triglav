#include "RotationTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "RenderViewport.hpp"
#include "src/RootWindow.hpp"

#include <spdlog/spdlog.h>

namespace triglav::editor {

RotationTool::RotationTool(LevelEditor& levelEditor) :
    m_levelEditor(levelEditor)
{
}

bool RotationTool::on_use_start(const geometry::Ray& ray)
{
   if (!m_rotationAxis.has_value())
      return false;

   const auto* object = m_levelEditor.selected_object();
   const auto& mesh = m_levelEditor.root_window().resource_manager().get(object->model);
   auto centroid = mesh.boundingBox.centroid();
   auto translation = object->transform.translation + centroid * object->transform.scale;

   auto point = find_point_on_aa_surface(ray.origin, ray.direction, *m_rotationAxis, vector3_component(translation, *m_rotationAxis));
   const auto difference = normalize(point - translation);

   auto euler = object->transform.rotation;

   switch (*m_rotationAxis) {
   case Axis::X:
      m_oldAngle = euler.x;
      m_baseAngle = std::atan2(difference.z, difference.y);
      break;
   case Axis::Y:
      m_oldAngle = euler.y;
      m_baseAngle = std::atan2(difference.z, difference.x);
      break;
   case Axis::Z:
      m_oldAngle = euler.z;
      m_baseAngle = std::atan2(difference.y, difference.x);
      break;
   }

   m_isBeingUsed = true;

   return true;
}

void RotationTool::on_mouse_moved(Vector2 position)
{
   const auto normalized_pos = position / rect_size(m_levelEditor.viewport().dimensions());
   const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);
   const auto ray = m_levelEditor.scene().camera().viewport_ray(viewport_coord);

   if (m_isBeingUsed) {
      assert(m_rotationAxis.has_value());
      const auto* object = m_levelEditor.selected_object();
      const auto& mesh = m_levelEditor.root_window().resource_manager().get(object->model);
      auto centroid = mesh.boundingBox.centroid();
      auto translation = object->transform.translation + centroid * object->transform.scale;

      auto point = find_point_on_aa_surface(ray.origin, ray.direction, *m_rotationAxis, vector3_component(translation, *m_rotationAxis));
      const auto difference = normalize(point - translation);

      float angle{};
      switch (*m_rotationAxis) {
      case Axis::X:
         angle = std::atan2(difference.z, difference.y);
         break;
      case Axis::Y:
         angle = std::atan2(difference.z, difference.x);
         break;
      case Axis::Z:
         angle = std::atan2(difference.y, difference.x);
         break;
      }

      const float angle_diff = angle - m_baseAngle;

      auto transform = m_levelEditor.selected_object()->transform;
      vector3_component(transform.rotation, *m_rotationAxis) = m_oldAngle + angle_diff;
      spdlog::info("Euler components {} {} {}", transform.rotation.x, transform.rotation.y, transform.rotation.z);
      m_levelEditor.scene().set_transform(m_levelEditor.selected_object_id(), transform);
      return;
   }

   bool intersects = false;
   float min_distance = INFINITY;
   Axis axis{};

   const auto ix = m_rotator_x_bb.intersect(ray);
   const auto iy = m_rotator_y_bb.intersect(ray);
   const auto iz = m_rotator_z_bb.intersect(ray);

   if (ix.has_value()) {
      intersects = true;
      min_distance = ix->x;
      axis = Axis::X;
   }
   if (iy.has_value()) {
      intersects = true;
      if (iy->x < min_distance) {
         min_distance = iy->x;
         axis = Axis::Y;
      }
   }
   if (iz.has_value()) {
      intersects = true;
      if (iz->x < min_distance) {
         min_distance = iz->x;
         axis = Axis::Z;
      }
   }

   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_X, COLOR_X_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Y, COLOR_Y_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Z, COLOR_Z_AXIS);

   if (intersects) {
      switch (axis) {
      case Axis::X:
         m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_X, COLOR_X_AXIS_HOVER);
         break;
      case Axis::Y:
         m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Y, COLOR_Y_AXIS_HOVER);
         break;
      case Axis::Z:
         m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Z, COLOR_Z_AXIS_HOVER);
         break;
      }
      m_rotationAxis = axis;
   }
}

void RotationTool::on_view_updated()
{
   static constexpr geometry::BoundingBox rotator_bb{
      .min{-ROTATOR_RADIUS, -ROTATOR_RADIUS, -ROTATOR_WIDTH},
      .max{ROTATOR_RADIUS, ROTATOR_RADIUS, ROTATOR_WIDTH},
   };

   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_X, COLOR_X_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Y, COLOR_Y_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Z, COLOR_Z_AXIS);

   const auto* object = m_levelEditor.selected_object();
   const auto& mesh = m_levelEditor.root_window().resource_manager().get(object->model);
   auto centroid = mesh.boundingBox.centroid();
   auto translation = object->transform.translation + centroid * object->transform.scale;

   const auto obj_distance = glm::length(object->transform.translation - m_levelEditor.scene().camera().position());

   const Transform3D transform_x_axis{
      .rotation = Vector3{0.5 * g_pi, 0.5 * g_pi, 0},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, transform_x_axis.to_matrix());
   m_rotator_x_bb = rotator_bb.transform(transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = Vector3{0.5 * g_pi, 0, 0},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, transform_y_axis.to_matrix());
   m_rotator_y_bb = rotator_bb.transform(transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = Vector3{0, 0, 0},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, transform_z_axis.to_matrix());
   m_rotator_z_bb = rotator_bb.transform(transform_z_axis.to_matrix());
}

void RotationTool::on_use_end()
{
   m_isBeingUsed = false;
}

void RotationTool::on_left_tool()
{
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, Matrix4x4{0});
}

}// namespace triglav::editor
