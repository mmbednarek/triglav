#include "ScalingTool.hpp"

#include "../RootWindow.hpp"
#include "LevelViewport.hpp"

namespace triglav::editor {

constexpr auto MIN_SCALE = 0.001f;

ScalingTool::ScalingTool(LevelEditor& levelEditor) :
    m_levelEditor(levelEditor)
{
}

bool ScalingTool::on_use_start(const geometry::Ray& ray)
{
   if (m_scaler_x_bb.intersect(ray)) {
      m_transformAxis = Axis::X;
   } else if (m_scaler_y_bb.intersect(ray)) {
      m_transformAxis = Axis::Y;
   } else if (m_scaler_z_bb.intersect(ray)) {
      m_transformAxis = Axis::Z;
   } else {
      return false;
   }

   const auto* object = m_levelEditor.selected_object();
   const auto translation = m_levelEditor.selected_object_position();
   const auto closest = find_closest_point_between_lines(translation, axis_forward_vec3(*m_transformAxis), ray.origin, ray.direction);
   m_startingTranslation = object->transform.translation;
   m_startingPosition = translation;
   m_startingClosest = closest;
   m_baseScale = object->transform.scale;
   return true;
}

void ScalingTool::on_mouse_moved(const Vector2 position)
{
   const auto normalized_pos = position / rect_size(m_levelEditor.viewport().dimensions());
   const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);
   const auto ray = m_levelEditor.scene().camera().viewport_ray(viewport_coord);

   if (m_transformAxis.has_value()) {
      const auto* object = m_levelEditor.selected_object();
      const auto& mesh = m_levelEditor.root_window().resource_manager().get(object->model);
      const auto translation = m_levelEditor.selected_object_position();

      auto transform = object->transform;
      const auto scale = 0.5f * mesh.boundingBox.scale() * m_baseScale;

      auto x1 = vector3_component(m_startingClosest - translation, *m_transformAxis);

      const auto closest = find_closest_point_between_lines(translation, axis_forward_vec3(*m_transformAxis), ray.origin, ray.direction);
      auto x2 = vector3_component(closest - translation, *m_transformAxis);

      const auto scale_comp = vector3_component(scale, *m_transformAxis);
      const auto diff = std::max(std::abs(x2 - x1), MIN_SCALE - scale_comp);
      const auto final_scale = (scale_comp + diff) / scale_comp;

      Vector3 translation_diff = m_startingTranslation - m_startingPosition;
      vector3_component(translation_diff, *m_transformAxis) *= final_scale;

      transform.translation = m_startingPosition + translation_diff;
      transform.scale = m_baseScale;
      vector3_component(transform.scale, *m_transformAxis) *= final_scale;
      m_levelEditor.scene().set_transform(m_levelEditor.selected_object_id(), transform);
      m_levelEditor.viewport().update_view();
      return;
   }

   if (m_scaler_x_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_X, COLOR_X_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_X, COLOR_X_AXIS);
   }

   if (m_scaler_y_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Y, COLOR_Y_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Y, COLOR_Y_AXIS);
   }

   if (m_scaler_z_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Z, COLOR_Z_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_SCALER_Z, COLOR_Z_AXIS);
   }
}

void ScalingTool::on_view_updated()
{
   static constexpr geometry::BoundingBox scaler_bb{
      .min{-BOX_SIZE, -BOX_SIZE, 0.0f},
      .max{BOX_SIZE, BOX_SIZE, BOX_SIZE + SHAFT_HEIGHT},
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
}

void ScalingTool::on_use_end()
{
   m_transformAxis.reset();
}

void ScalingTool::on_left_tool()
{
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_X, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_Y, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_SCALER_Z, Matrix4x4{0});
}

}// namespace triglav::editor
