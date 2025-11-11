#pragma once

#include "ILevelEditorTool.hpp"
#include "RotationTool.hpp"
#include "ScalingTool.hpp"
#include "SelectionTool.hpp"
#include "TranslationTool.hpp"

#include "triglav/desktop_ui/CheckBox.hpp"
#include "triglav/desktop_ui/DropDownMenu.hpp"
#include "triglav/renderer/BindlessScene.hpp"
#include "triglav/renderer/OcclusionCulling.hpp"
#include "triglav/renderer/RenderingJob.hpp"
#include "triglav/renderer/Scene.hpp"
#include "triglav/renderer/UpdateViewParamsJob.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::editor {

class LevelViewport;
class RootWindow;

class LevelEditor final : public ui_core::ProxyWidget
{
   friend class RenderViewport;

 public:
   using Self = LevelEditor;
   struct State
   {
      desktop_ui::DesktopUIManager* manager;
      RootWindow* rootWindow;
   };

   LevelEditor(ui_core::Context& context, State state, ui_core::IWidget* parent);

   [[nodiscard]] renderer::Scene& scene();
   void tick(float delta_time);
   const renderer::SceneObject* selected_object() const;
   renderer::ObjectID selected_object_id() const;
   [[nodiscard]] Vector3 selected_object_position(std::optional<Vector3> position = std::nullopt) const;
   LevelViewport& viewport() const;
   ILevelEditorTool& tool() const;
   SelectionTool& selection_tool();
   [[nodiscard]] RootWindow& root_window() const;
   void on_selected_tool(u32 id);
   void on_origin_selected(u32 id) const;
   float snap_offset(float offset) const;
   Vector3 snap_offset(Vector3 offset) const;
   float speed() const;

   void set_selected_object(renderer::ObjectID id);

 private:
   State m_state;
   desktop_ui::RadioGroup m_toolRadioGroup;
   renderer::Scene m_scene;
   renderer::BindlessScene m_bindlessScene;
   renderer::Config m_config;
   renderer::UpdateViewParamsJob m_updateViewParamsJob;
   renderer::OcclusionCulling m_occlusionCulling;
   renderer::RenderingJob m_renderingJob;
   const renderer::SceneObject* m_selectedObject{};
   renderer::ObjectID m_selectedObjectID{};
   LevelViewport* m_viewport;
   SelectionTool m_selectionTool;
   TranslationTool m_translationTool;
   RotationTool m_rotationTool;
   ScalingTool m_scalingTool;
   desktop_ui::DropDownMenu* m_originSelector;
   desktop_ui::DropDownMenu* m_snapSelector;
   desktop_ui::DropDownMenu* m_speedSelector;

   ILevelEditorTool* m_currentTool = nullptr;

   TG_SINK(desktop_ui::RadioGroup, OnSelection);
   TG_OPT_SINK(desktop_ui::DropDownMenu, OnSelected);
};

}// namespace triglav::editor
