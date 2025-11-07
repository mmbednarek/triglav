#include "TranslationTool.hpp"

#include "LevelViewport.hpp"

namespace triglav::editor {

TranslationTool::TranslationTool(LevelEditor& levelEditor) :
    m_levelEditor(levelEditor)
{
}

bool TranslationTool::on_use_start(const geometry::Ray& ray)
{
   if (m_arrow_x_bb.intersect(ray)) {
      m_transformAxis = Axis::X;
   } else if (m_arrow_y_bb.intersect(ray)) {
      m_transformAxis = Axis::Y;
   } else if (m_arrow_z_bb.intersect(ray)) {
      m_transformAxis = Axis::Z;
   } else {
      return false;
   }

   const auto& transform = m_levelEditor.selected_object()->transform;
   const auto closest =
      find_closest_point_between_lines(transform.translation, axis_forward_vec3(*m_transformAxis), ray.origin, ray.direction);
   m_translationOffset = transform.translation - closest;
   return true;
}

void TranslationTool::on_mouse_moved(const Vector2 position)
{
   const auto normalized_pos = position / rect_size(m_levelEditor.viewport().dimensions());
   const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);
   const auto ray = m_levelEditor.scene().camera().viewport_ray(viewport_coord);

   if (m_transformAxis.has_value()) {
      auto transform = m_levelEditor.selected_object()->transform;
      const auto closest =
         find_closest_point_between_lines(transform.translation, axis_forward_vec3(*m_transformAxis), ray.origin, ray.direction);
      transform.translation = closest + m_translationOffset;
      m_levelEditor.scene().set_transform(m_levelEditor.selected_object_id(), transform);
      m_levelEditor.viewport().update_view();
      return;
   }

   if (m_arrow_x_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(1, COLOR_X_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(1, COLOR_X_AXIS);
   }

   if (m_arrow_y_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(2, COLOR_Y_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(2, COLOR_Y_AXIS);
   }

   if (m_arrow_z_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(3, COLOR_Z_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(3, COLOR_Z_AXIS);
   }
}

void TranslationTool::on_view_updated()
{
   static constexpr geometry::BoundingBox arrow_bb{
      .min{-TIP_RADIUS, -TIP_RADIUS, 0.0f},
      .max{TIP_RADIUS, TIP_RADIUS, ARROW_HEIGHT},
   };

   const auto* object = m_levelEditor.selected_object();
   const auto obj_distance = glm::length(object->transform.translation - m_levelEditor.scene().camera().position());

   const Transform3D transform_x_axis{
      .rotation = Vector3{0.5 * g_pi, 0.5 * g_pi, 0},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = object->transform.translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(1, transform_x_axis.to_matrix());
   m_arrow_x_bb = arrow_bb.transform(transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = Vector3{0.5 * g_pi, 0, 0},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = object->transform.translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(2, transform_y_axis.to_matrix());
   m_arrow_y_bb = arrow_bb.transform(transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = Vector3{g_pi, 0, 0},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = object->transform.translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(3, transform_z_axis.to_matrix());
   m_arrow_z_bb = arrow_bb.transform(transform_z_axis.to_matrix());
}

void TranslationTool::on_use_end()
{
   m_transformAxis.reset();
}

void TranslationTool::on_left_tool()
{
   m_levelEditor.viewport().render_viewport().set_selection_matrix(1, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(2, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(3, Matrix4x4{0});
}

}// namespace triglav::editor