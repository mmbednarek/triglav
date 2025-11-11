#include "TranslationTool.hpp"

#include "../../../../../../../.conan2/p/b/boost685345d9ed20d/p/include/boost/algorithm/minmax_element.hpp"
#include "../RootWindow.hpp"
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

   m_startingPosition = m_levelEditor.selected_object()->transform.translation;
   m_startingHit = find_closest_point_between_lines(m_levelEditor.selected_object_position(), axis_forward_vec3(*m_transformAxis),
                                                    ray.origin, ray.direction);
   return true;
}

void TranslationTool::on_mouse_moved(const Vector2 position)
{
   const auto normalized_pos = position / rect_size(m_levelEditor.viewport().dimensions());
   const auto viewport_coord = 2.0f * normalized_pos - Vector2(1, 1);
   const auto ray = m_levelEditor.scene().camera().viewport_ray(viewport_coord);

   if (m_transformAxis.has_value()) {
      const auto* object = m_levelEditor.selected_object();
      auto transform = object->transform;
      transform.translation =
         m_startingPosition +
         m_levelEditor.snap_offset(find_closest_point_between_lines(m_levelEditor.selected_object_position(),
                                                                    axis_forward_vec3(*m_transformAxis), ray.origin, ray.direction) -
                                   m_startingHit);
      m_levelEditor.scene().set_transform(m_levelEditor.selected_object_id(), transform);
      m_levelEditor.viewport().update_view();
      return;
   }

   if (m_arrow_x_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ARROW_X, COLOR_X_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ARROW_X, COLOR_X_AXIS);
   }

   if (m_arrow_y_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ARROW_Y, COLOR_Y_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ARROW_Y, COLOR_Y_AXIS);
   }

   if (m_arrow_z_bb.intersect(ray)) {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ARROW_Z, COLOR_Z_AXIS_HOVER);
   } else {
      m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ARROW_Z, COLOR_Z_AXIS);
   }
}

void TranslationTool::on_view_updated()
{
   static constexpr geometry::BoundingBox arrow_bb{
      .min{-TIP_RADIUS, -TIP_RADIUS, 0.0f},
      .max{TIP_RADIUS, TIP_RADIUS, ARROW_HEIGHT},
   };

   const auto translation = m_levelEditor.selected_object_position();
   const auto obj_distance = glm::length(translation - m_levelEditor.scene().camera().position());

   const Transform3D transform_x_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0.5 * g_pi}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_X, transform_x_axis.to_matrix());
   m_arrow_x_bb = arrow_bb.transform(transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = Quaternion{Vector3{0.5 * g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Y, transform_y_axis.to_matrix());
   m_arrow_y_bb = arrow_bb.transform(transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = Quaternion{Vector3{g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = translation,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Z, transform_z_axis.to_matrix());
   m_arrow_z_bb = arrow_bb.transform(transform_z_axis.to_matrix());
}

void TranslationTool::on_use_end()
{
   m_transformAxis.reset();
}

void TranslationTool::on_left_tool()
{
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_X, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Y, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ARROW_Z, Matrix4x4{0});
}

}// namespace triglav::editor