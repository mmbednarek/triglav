#include "SetTransformAction.hpp"

#include "LevelEditor.hpp"
#include "LevelViewport.hpp"

namespace triglav::editor {

SetTransformAction::SetTransformAction(LevelEditor& level_editor, renderer::ObjectID object_id, const Transform3D& previous_transform,
                                       const Transform3D& transform) :
    m_level_editor(level_editor),
    m_object_id(object_id),
    m_previous_transform(previous_transform),
    m_transform(transform)
{
   assert(m_object_id != renderer::UNSELECTED_OBJECT);
   log_info("Inserting action, object_id: {}", m_object_id);
}

void SetTransformAction::redo()
{
   log_info("Redo: object_id: {}", m_object_id);
   m_level_editor.scene().set_transform(m_object_id, m_transform);
   m_level_editor.viewport().update_view();
}

void SetTransformAction::undo()
{
   log_info("Undo: object_id: {}", m_object_id);
   m_level_editor.scene().set_transform(m_object_id, m_previous_transform);
   m_level_editor.viewport().update_view();
}

}// namespace triglav::editor
