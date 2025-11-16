#include "ScalingTool.hpp"

#include "../RootWindow.hpp"
#include "LevelViewport.hpp"
#include "SetTransformAction.hpp"

namespace triglav::editor {

constexpr auto MIN_SCALE = 0.001f;

ScalingTool::ScalingTool(LevelEditor& levelEditor) :
    m_levelEditor(levelEditor)
{
}

bool ScalingTool::on_use_start(const geometry::Ray& ray)
{
   if (!m_transformAxis.has_value())
      return false;

   m_isBeingUsed = true;

   const auto* object = m_levelEditor.selected_object();
   const auto translation = m_levelEditor.selected_object_position();
   const auto closest = find_closest_point_between_lines(translation, axis_forward_vec3(*m_transformAxis), ray.origin, ray.direction);
   m_startingTransform = object->transform;
   m_startingPosition = translation;
   m_startingPoint = find_closest_point_on_line(ray.origin, ray.direction, translation);
   m_startingClosest = closest;
   return true;
}

void ScalingTool::on_mouse_moved(const Vector2 position)
{
   const auto normalized_pos = position / rect_size(m_levelEditor.viewport().dimensions());
   const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);
   const auto ray = m_levelEditor.scene().camera().viewport_ray(viewport_coord);

   if (m_isBeingUsed) {
      const auto axis = m_transformAxis.value();

      const auto* object = m_levelEditor.selected_object();
      const auto& mesh = m_levelEditor.root_window().resource_manager().get(object->model);
      const auto translation = m_levelEditor.selected_object_position();
      auto transform = object->transform;
      const auto scale = 0.5f * mesh.boundingBox.scale() * m_startingTransform.scale;

      Vector3 scale_direction = {1, 0, 0};
      float final_scale = 0.0f;

      if (axis == Axis::W) {
         scale_direction = glm::normalize(Vector3{1, 1, 1});

         auto x1 = glm::length(m_startingPoint - m_startingPosition);

         const auto point_position = find_closest_point_on_line(ray.origin, ray.direction, m_startingPosition);
         auto x2 = glm::length(point_position - m_startingPosition);
         const auto diff = std::max(x2 - x1, MIN_SCALE - 1.0f);
         final_scale = m_levelEditor.snap_offset(diff);
      } else {
         scale_direction = axis_forward_vec3(axis);

         auto x1 = std::abs(vector3_component(m_startingClosest - translation, axis));
         const auto closest = find_closest_point_between_lines(translation, scale_direction, ray.origin, ray.direction);
         auto x2 = std::abs(vector3_component(closest - translation, axis));
         const auto scale_comp = vector3_component(scale, axis);

         const auto diff = std::max(x2 - x1, MIN_SCALE - scale_comp);
         final_scale = m_levelEditor.snap_offset((scale_comp + diff) / scale_comp) - 1.0f;

         auto rot_inv = glm::inverse(transform.rotation);
         scale_direction = rot_inv * scale_direction;
         scale_direction = glm::abs(scale_direction);
      }

      Vector3 scale_vec = scale_direction * final_scale + Vector3{1, 1, 1};

      Vector3 translation_diff = (m_startingTransform.translation - m_startingPosition) * scale_vec;

      transform.translation = m_startingPosition + translation_diff;
      transform.scale = m_startingTransform.scale * scale_vec;
      m_levelEditor.set_selected_transform(transform);
      m_levelEditor.viewport().update_view();
      return;
   }

   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_X, COLOR_X_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Y, COLOR_Y_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Z, COLOR_Z_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_XYZ, COLOR_XYZ_AXIS);

   if (m_transformAxis = get_transform_axis(ray); m_transformAxis.has_value()) {
      switch (*m_transformAxis) {
      case Axis::X:
         m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_X, COLOR_X_AXIS_HOVER);
         break;
      case Axis::Y:
         m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Y, COLOR_Y_AXIS_HOVER);
         break;
      case Axis::Z:
         m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Z, COLOR_Z_AXIS_HOVER);
         break;
      case Axis::W:
         m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_XYZ, COLOR_XYZ_AXIS_HOVER);
         break;
      }
   }
}

void ScalingTool::on_view_updated()
{
   static constexpr geometry::BoundingBox scaler_bb{
      .min{-BOX_SIZE, -BOX_SIZE, 0.0f},
      .max{BOX_SIZE, BOX_SIZE, BOX_SIZE + SHAFT_HEIGHT},
   };

   static constexpr geometry::BoundingBox scaler_all_axis_bb{
      .min{-4 * SCALING_CUBE, -4 * SCALING_CUBE, -4 * SCALING_CUBE},
      .max{4 * SCALING_CUBE, 4 * SCALING_CUBE, 4 * SCALING_CUBE},
   };

   const auto translation = m_levelEditor.selected_object_position();
   const auto obj_distance = glm::length(translation - m_levelEditor.scene().camera().position());

   const Transform3D transform_x_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0.5 * g_pi}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_X, transform_x_axis.to_matrix());
   m_scaler_x_bb = scaler_bb.transform(transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_Y, transform_y_axis.to_matrix());
   m_scaler_y_bb = scaler_bb.transform(transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = Quaternion{Vector3{g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_Z, transform_z_axis.to_matrix());
   m_scaler_z_bb = scaler_bb.transform(transform_z_axis.to_matrix());

   const Transform3D transform_xyz_axis{
      .rotation = Quaternion{Vector3{0, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_XYZ, transform_xyz_axis.to_matrix());
   m_scaler_xyz_bb = scaler_all_axis_bb.transform(transform_xyz_axis.to_matrix());
}

void ScalingTool::on_use_end()
{
   if (m_isBeingUsed) {
      m_isBeingUsed = false;
      m_transformAxis.reset();

      m_levelEditor.history_manager().emplace_action<SetTransformAction>(m_levelEditor, m_levelEditor.selected_object_id(),
                                                                         m_startingTransform, m_levelEditor.selected_object()->transform);
   }
}

void ScalingTool::on_left_tool()
{
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_X, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_Y, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_Z, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_XYZ, Matrix4x4{0});
}

std::optional<Axis> ScalingTool::get_transform_axis(const geometry::Ray& ray) const
{
   if (m_scaler_xyz_bb.intersect(ray)) {
      return Axis::W;
   }
   if (m_scaler_x_bb.intersect(ray)) {
      return Axis::X;
   }
   if (m_scaler_y_bb.intersect(ray)) {
      return Axis::Y;
   }
   if (m_scaler_z_bb.intersect(ray)) {
      return Axis::Z;
   }

   return std::nullopt;
}

}// namespace triglav::editor
