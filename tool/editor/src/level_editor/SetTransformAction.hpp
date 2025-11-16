#pragma once

#include "../HistoryManager.hpp"
#include "triglav/Logging.hpp"
#include "triglav/Math.hpp"
#include "triglav/renderer/Scene.hpp"

namespace triglav::editor {

class LevelEditor;

class SetTransformAction final : public IHistoryAction
{
   TG_DEFINE_LOG_CATEGORY(SetTransformAction)
 public:
   SetTransformAction(LevelEditor& level_editor, renderer::ObjectID object_id, const Transform3D& previous_transform,
                      const Transform3D& transform);

   void redo() override;
   void undo() override;

 private:
   LevelEditor& m_level_editor;
   renderer::ObjectID m_object_id;
   Transform3D m_previous_transform;
   Transform3D m_transform;
};

}// namespace triglav::editor