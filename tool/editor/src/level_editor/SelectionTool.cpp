#include "SelectionTool.hpp"

#include "triglav/physics/PhysicsSystem.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "src/RootWindow.hpp"

namespace triglav::editor {

SelectionTool::SelectionTool(LevelEditor& level_editor) :
    m_level_editor(level_editor)
{
}

bool SelectionTool::on_use_start(const geometry::Ray& ray)
{
   const auto hit = m_level_editor.level().system<physics::PhysicsSystem>().trace_ray(ray);
   if (hit.entity_id != world::NO_ENTITY) {
      m_level_editor.set_selected_object(hit.entity_id);
   }
   return true;
}

void SelectionTool::on_mouse_moved(Vector2 /*position*/) {}

void SelectionTool::on_view_updated()
{
   const auto* transform_ptr = m_level_editor.selected_object_transform();
   if (transform_ptr == nullptr)
      return;

   const auto* mesh_component_ptr = m_level_editor.level().component_opt<world::Mesh>(m_level_editor.selected_object_id());
   if (mesh_component_ptr == nullptr)
      return;

   const auto& mesh = m_level_editor.root_window().resource_manager().get(mesh_component_ptr->name);
   auto corrected_bb = mesh.bounding_box.transform(transform_ptr->to_matrix());

   const Transform3D select_transform{
      .rotation = {1, 0, 0, 0},
      .scale = corrected_bb.scale(),
      .translation = corrected_bb.min,
   };
   m_level_editor.viewport().render_viewport().set_selection_matrix(0, select_transform.to_matrix());
}

void SelectionTool::on_use_end() {}

void SelectionTool::on_left_tool() {}

}// namespace triglav::editor
