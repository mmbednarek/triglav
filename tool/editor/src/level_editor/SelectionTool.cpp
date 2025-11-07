#include "SelectionTool.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"
#include "src/RootWindow.hpp"

namespace triglav::editor {

SelectionTool::SelectionTool(LevelEditor& levelEditor) :
    m_levelEditor(levelEditor)
{
}

bool SelectionTool::on_use_start(const geometry::Ray& ray)
{
   const auto hit = m_levelEditor.scene().trace_ray(ray);
   if (hit.object != nullptr) {
      m_levelEditor.set_selected_object(hit.id);
      m_levelEditor.viewport().update_view();
   }
   return true;
}

void SelectionTool::on_mouse_moved(Vector2 /*position*/) {}

void SelectionTool::on_view_updated()
{
   const renderer::SceneObject* object = m_levelEditor.selected_object();

   const auto& mesh = m_levelEditor.root_window().resource_manager().get(object->model);
   const Transform3D select_transform{
      .rotation = {0, 0, 0},
      .scale = object->transform.scale * mesh.boundingBox.scale(),
      .translation = object->transform.translation + mesh.boundingBox.min * object->transform.scale,
   };
   m_levelEditor.viewport().render_viewport().set_selection_matrix(0, select_transform.to_matrix());
}

void SelectionTool::on_use_end() {}

void SelectionTool::on_left_tool() {}

}// namespace triglav::editor
