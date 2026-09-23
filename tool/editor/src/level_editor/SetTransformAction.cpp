#include "SetTransformAction.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"

namespace triglav::editor {

SetTransformAction::SetTransformAction(LevelEditor& level_editor, world::EntityID object_id, const Transform3D& previous_transform,
                                       const Transform3D& transform) :
    m_level_editor(level_editor),
    m_object_id(object_id),
    m_previous_transform(previous_transform),
    m_transform(transform)
{
   assert(m_object_id != world::NO_ENTITY);
   log_debug("Inserting action, object_id: {}", m_object_id);
}

void SetTransformAction::redo()
{
   log_debug("Redo: object_id: {}", m_object_id);
   auto* transform_ptr = m_level_editor.level().mut_component_opt<Transform3D>(m_object_id);
   if (transform_ptr == nullptr) {
      log_error("Failed to retrieve transform for entity {}", m_object_id);
      return;
   }

   *transform_ptr = m_transform;

   m_level_editor.viewport().update_view();
}

void SetTransformAction::undo()
{
   log_debug("Undo: object_id: {}", m_object_id);
   auto* transform_ptr = m_level_editor.level().mut_component_opt<Transform3D>(m_object_id);
   if (transform_ptr == nullptr) {
      log_error("Failed to retrieve transform for entity {}", m_object_id);
      return;
   }

   *transform_ptr = m_transform;
   m_level_editor.viewport().update_view();
}

}// namespace triglav::editor
