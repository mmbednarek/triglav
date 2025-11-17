#include "RotationTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "RenderViewport.hpp"
#include "SetTransformAction.hpp"
#include "src/RootWindow.hpp"

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

RotationTool::RotationTool(LevelEditor& level_editor) :
    m_level_editor(level_editor)
{
}

bool RotationTool::on_use_start(const geometry::Ray& ray)
{
   if (!m_rotation_axis.has_value())
      return false;

   const auto* object = m_level_editor.selected_object();
   auto position = m_level_editor.selected_object_position();

   const auto point = find_point_on_aa_surface(ray.origin, ray.direction, *m_rotation_axis, vector3_component(position, *m_rotation_axis));
   const auto difference = normalize(point - position);

   m_starting_transform = object->transform;
   m_base_angle = angle_from_vector(difference, *m_rotation_axis);
   m_is_being_used = true;

   return true;
}

void RotationTool::on_mouse_moved(Vector2 position)
{
   const auto normalized_pos = position / rect_size(m_level_editor.viewport().dimensions());
   const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);
   const auto ray = m_level_editor.scene().camera().viewport_ray(viewport_coord);

   if (m_is_being_used) {
      assert(m_rotation_axis.has_value());
      const auto* object = m_level_editor.selected_object();
      const auto obj_position = m_level_editor.selected_object_position(m_starting_transform.translation);

      auto point = find_point_on_aa_surface(ray.origin, ray.direction, *m_rotation_axis, vector3_component(obj_position, *m_rotation_axis));
      const auto difference = glm::normalize(point - obj_position);
      const float angle = angle_from_vector(difference, *m_rotation_axis);
      const float angle_diff = m_level_editor.snap_offset((angle - m_base_angle) / (0.25f * g_pi)) * (0.25f * g_pi);

      auto quat_rot = glm::rotate(glm::quat{1, 0, 0, 0}, angle_diff, axis_forward_vec3(*m_rotation_axis));

      auto transform = object->transform;
      transform.translation = obj_position + quat_rot * (m_starting_transform.translation - obj_position);
      transform.rotation = quat_rot * m_starting_transform.rotation;
      m_level_editor.set_selected_transform(transform);
      m_level_editor.viewport().update_view();
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

   m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_X, COLOR_X_AXIS);
   m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Y, COLOR_Y_AXIS);
   m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Z, COLOR_Z_AXIS);

   if (intersects) {
      switch (axis) {
      case Axis::X:
         m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_X, COLOR_X_AXIS_HOVER);
         break;
      case Axis::Y:
         m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Y, COLOR_Y_AXIS_HOVER);
         break;
      case Axis::Z:
         m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Z, COLOR_Z_AXIS_HOVER);
         break;
      default:
         break;
      }
      m_rotation_axis = axis;
   } else {
      m_rotation_axis.reset();
   }
}

void RotationTool::on_view_updated()
{
   static constexpr geometry::BoundingBox rotator_bb{
      .min{-ROTATOR_RADIUS, -ROTATOR_RADIUS, -ROTATOR_WIDTH},
      .max{ROTATOR_RADIUS, ROTATOR_RADIUS, ROTATOR_WIDTH},
   };

   m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_X, COLOR_X_AXIS);
   m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Y, COLOR_Y_AXIS);
   m_level_editor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Z, COLOR_Z_AXIS);

   auto obj_position = m_level_editor.selected_object_position();
   const auto obj_distance = glm::length(obj_position - m_level_editor.scene().camera().position());

   const Transform3D transform_x_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0.5 * g_pi}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = obj_position,
   };
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, transform_x_axis.to_matrix());
   m_rotator_x_bb = rotator_bb.transform(transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = obj_position,
   };
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, transform_y_axis.to_matrix());
   m_rotator_y_bb = rotator_bb.transform(transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = Quaternion{Vector3{0, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = obj_position,
   };
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, transform_z_axis.to_matrix());
   m_rotator_z_bb = rotator_bb.transform(transform_z_axis.to_matrix());
}

void RotationTool::on_use_end()
{
   if (m_is_being_used) {
      m_is_being_used = false;
      m_rotation_axis.reset();

      m_level_editor.history_manager().emplace_action<SetTransformAction>(
         m_level_editor, m_level_editor.selected_object_id(), m_starting_transform, m_level_editor.selected_object()->transform);
   }
}

void RotationTool::on_left_tool()
{
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, Matrix4x4{0});
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, Matrix4x4{0});
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, Matrix4x4{0});
}

}// namespace triglav::editor
