#pragma once

#include "ILevelEditorTool.hpp"

namespace triglav::editor {

class LevelEditor;

class SelectionTool final : public ILevelEditorTool
{
 public:
   explicit SelectionTool(LevelEditor& level_editor);

   bool on_use_start(const geometry::Ray& ray) override;
   void on_mouse_moved(Vector2 position) override;
   void on_view_updated() override;
   void on_use_end() override;
   void on_left_tool() override;

 private:
   LevelEditor& m_level_editor;
};

}// namespace triglav::editor
// Created by ego on 6.11.2025.
//

#ifndef TRIGLAV_SELECTIONTOOL_HPP
#define TRIGLAV_SELECTIONTOOL_HPP

#endif// TRIGLAV_SELECTIONTOOL_HPP
