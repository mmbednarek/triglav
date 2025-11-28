#include "TranslationTool.hpp"

#include "../RootWindow.hpp"
#include "LevelViewport.hpp"
#include "SetTransformAction.hpp"

namespace triglav::editor {

TranslationTool::TranslationTool(LevelEditor& level_editor) :
    m_level_editor(level_editor)
{
}

bool TranslationTool::on_use_start(const geometry::Ray& ray)
{
   if (m_arrow_x_bb.intersect(ray)) {
      m_transform_axis = Axis::X;
   } else if (m_arrow_y_bb.intersect(ray)) {
      m_transform_axis = Axis::Y;
   } else if (m_arrow_z_bb.intersect(ray)) {
      m_transform_axis = Axis::Z;
   } else {
      return false;
   }

   m_starting_transform = m_level_editor.selected_object()->transform;
   m_starting_hit = find_closest_point_between_lines(m_level_editor.selected_object_position(), axis_forward_vec3(*m_transform_axis),
                                                     ray.origin, ray.direction);
   return true;
}

void TranslationTool::on_mouse_moved(const Vector2 position)
{
   const auto normalized_pos = position / rect_size(m_level_editor.viewport().dimensions());
   const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);
   const auto ray = m_level_editor.scene().camera().viewport_ray(viewport_coord);

   if (m_transform_axis.has_value()) {
      const auto* object = m_level_editor.selected_object();
      auto transform = object->transform;
      transform.translation =
         m_starting_transform.translation +
         m_level_editor.snap_offset(find_closest_point_between_lines(m_level_editor.selected_object_position(),
                                                                     axis_forward_vec3(*m_transform_axis), ray.origin, ray.direction) -
                                    m_starting_hit);
      m_level_editor.set_selected_transform(transform);
      return;
   }

   if (m_arrow_x_bb.intersect(ray)) {
      m_level_editor.viewport().render_viewport().set_color(OVERLAY_ARROW_X, COLOR_X_AXIS_HOVER);
   } else {
      m_level_editor.viewport().render_viewport().set_color(OVERLAY_ARROW_X, COLOR_X_AXIS);
   }

   if (m_arrow_y_bb.intersect(ray)) {
      m_level_editor.viewport().render_viewport().set_color(OVERLAY_ARROW_Y, COLOR_Y_AXIS_HOVER);
   } else {
      m_level_editor.viewport().render_viewport().set_color(OVERLAY_ARROW_Y, COLOR_Y_AXIS);
   }

   if (m_arrow_z_bb.intersect(ray)) {
      m_level_editor.viewport().render_viewport().set_color(OVERLAY_ARROW_Z, COLOR_Z_AXIS_HOVER);
   } else {
      m_level_editor.viewport().render_viewport().set_color(OVERLAY_ARROW_Z, COLOR_Z_AXIS);
   }
}

void TranslationTool::on_view_updated()
{
   static constexpr geometry::BoundingBox arrow_bb{
      .min{-TIP_RADIUS, -TIP_RADIUS, 0.0f},
      .max{TIP_RADIUS, TIP_RADIUS, ARROW_HEIGHT},
   };

   const auto translation = m_level_editor.selected_object_position();
   const auto obj_distance = glm::length(translation - m_level_editor.scene().camera().position());

   const Transform3D transform_x_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0.5 * g_pi}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_X, transform_x_axis.to_matrix());
   m_arrow_x_bb = arrow_bb.transform(transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Y, transform_y_axis.to_matrix());
   m_arrow_y_bb = arrow_bb.transform(transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = Quaternion{Vector3{g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Z, transform_z_axis.to_matrix());
   m_arrow_z_bb = arrow_bb.transform(transform_z_axis.to_matrix());
}

void TranslationTool::on_use_end()
{
   if (m_transform_axis.has_value()) {
      m_level_editor.history_manager().emplace_action<SetTransformAction>(
         m_level_editor, m_level_editor.selected_object_id(), m_starting_transform, m_level_editor.selected_object()->transform);
      m_transform_axis.reset();
   }
}

void TranslationTool::on_left_tool()
{
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_X, Matrix4x4{0});
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Y, Matrix4x4{0});
   m_level_editor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Z, Matrix4x4{0});
}

}// namespace triglav::editor