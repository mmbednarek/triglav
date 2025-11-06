#include "RotationTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "RenderViewport.hpp"
#include "src/RootWindow.hpp"

namespace triglav::editor {

RotationTool::RotationTool(LevelEditor& levelEditor) :
    m_levelEditor(levelEditor)
{
}

bool RotationTool::on_use_start(const geometry::Ray& /*ray*/)
{
   return false;
}

void RotationTool::on_mouse_moved(Vector2 /*position*/) {}

void RotationTool::on_view_updated()
{
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_X, COLOR_X_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Y, COLOR_Y_AXIS);
   m_levelEditor.viewport().render_viewport().set_color(OVERLAY_ROTATOR_Z, COLOR_Z_AXIS);

   const auto* object = m_levelEditor.selected_object();
   const auto& mesh = m_levelEditor.root_window().resource_manager().get(object->model);
   auto centroid = mesh.boundingBox.centroid();

   const auto obj_distance = glm::length(object->transform.translation - m_levelEditor.scene().camera().position());

   const Transform3D transform_x_axis{
      .rotation = glm::quat{Vector3{0.5 * g_pi, 0, 0.5 * g_pi}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = object->transform.translation + centroid * object->transform.scale,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, transform_x_axis.to_matrix());

   const Transform3D transform_y_axis{
      .rotation = glm::quat{Vector3{0.5 * g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = object->transform.translation + centroid * object->transform.scale,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, transform_y_axis.to_matrix());

   const Transform3D transform_z_axis{
      .rotation = glm::quat{Vector3{g_pi, 0, 0}},
      .scale = Vector3{0.025f} * obj_distance,
      .translation = object->transform.translation + centroid * object->transform.scale,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, transform_z_axis.to_matrix());
}

void RotationTool::on_use_end() {}

void RotationTool::on_left_tool()
{
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_X, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Y, Matrix4x4{0});
   m_levelEditor.viewport().render_viewport().set_selection_matrix(OVERLAY_ROTATOR_Z, Matrix4x4{0});
}

}// namespace triglav::editor
