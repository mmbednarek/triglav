#include "RotationTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "RenderViewport.hpp"
#include "src/RootWindow.hpp"

#include <spdlog/spdlog.h>

namespace triglav::editor {

namespace {

float angle_from_vector(const Vector3 vec, const Axis rotation_axis)
{
   switch (rotation_axis) {
   case Axis::X:
      return std::atan2(vec.z, vec.y);
      break;
   case Axis::Y:
      return std::atan2(vec.x, vec.z);
      break;
   case Axis::Z:
      return std::atan2(vec.y, vec.x);
      break;
   default:
      return 0.0f;
   }
}

}// namespace

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
   const auto centroid = mesh.boundingBox.centroid();
   const auto translation = object->transform.translation + centroid * object->transform.scale;

   const auto point = find_point_on_aa_surface(ray.origin, ray.direction, *m_rotationAxis, vector3_component(translation, *m_rotationAxis));
   const auto difference = normalize(point - translation);

   m_startingRotation = object->transform.rotation;
   m_baseAngle = angle_from_vector(difference, *m_rotationAxis);
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
      const float angle = angle_from_vector(difference, *m_rotationAxis);
      const float angle_diff = angle - m_baseAngle;

      auto transform = m_levelEditor.selected_object()->transform;
      transform.rotation = glm::rotate(glm::quat{1, 0, 0, 0}, angle_diff, axis_forward_vec3(*m_rotationAxis)) * m_startingRotation;
      m_levelEditor.scene().set_transform(m_levelEditor.selected_object_id(), transform);

      auto euler = glm::eulerAngles(transform.rotation);
      spdlog::info("Euler components {} {} {}", euler.x, euler.y, euler.z);

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
      default:
         break;
      }
      m_rotationAxis = axis;
   } else {
      m_rotationAxis.reset();
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
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0.5 * g_pi}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, transform_x_axis.to_matrix());
   m_rotator_x_bb = rotator_bb.transform(transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, transform_y_axis.to_matrix());
   m_rotator_y_bb = rotator_bb.transform(transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = Quaternion{Vector3{0, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, transform_z_axis.to_matrix());
   m_rotator_z_bb = rotator_bb.transform(transform_z_axis.to_matrix());
}

void RotationTool::on_use_end()
{
   m_isBeingUsed = false;
   m_rotationAxis.reset();
}

void RotationTool::on_left_tool()
{
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, Matrix4x4{0});
}

}// namespace triglav::editor
