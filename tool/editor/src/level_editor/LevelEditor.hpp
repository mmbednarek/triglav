#pragma once

#include "../Command.hpp"
#include "../HistoryManager.hpp"
#include "../IAssetEditor.hpp"
#include "ILevelEditorTool.hpp"
#include "RotationTool.hpp"
#include "ScalingTool.hpp"
#include "SelectionTool.hpp"
#include "TranslationTool.hpp"

#include "triglav/desktop_ui/CheckBox.hpp"
#include "triglav/desktop_ui/DropDownMenu.hpp"
#include "triglav/desktop_ui/TabView.hpp"
#include "triglav/renderer/BindlessScene.hpp"
#include "triglav/renderer/OcclusionCulling.hpp"
#include "triglav/renderer/RenderingJob.hpp"
#include "triglav/renderer/Scene.hpp"
#include "triglav/renderer/UpdateViewParamsJob.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::editor {

class LevelViewport;
class RootWindow;
class LevelEditorSidePanel;

enum class LevelEditorTool
{
   Selection,
   Translation,
   Rotation,
   Scaling,
};

class LevelEditor final : public ui_core::ProxyWidget, public desktop_ui::ITabWidget, public IAssetEditor
{
   friend class RenderViewport;

 public:
   using Self = LevelEditor;
   struct State
   {
      desktop_ui::DesktopUIManager* manager;
      RootWindow* root_window;
      ResourceName asset_name;
   };

   LevelEditor(ui_core::Context& context, State state, ui_core::IWidget* parent);

   [[nodiscard]] renderer::Scene& scene();
   void tick(float delta_time) override;
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
   void finish_using_tool() const;
   void set_selected_transform(const Transform3D& transform);
   void set_selected_name(StringView name);
   void set_selected_object(renderer::ObjectID id);
   HistoryManager& history_manager();
   void on_event(const ui_core::Event& event) override;
   void save_level() const;
   void remove_selected_item();
   void on_command(Command command) override;
   [[nodiscard]] bool accepts_key_chords() const override;
   void set_active_tool(LevelEditorTool tool);
   [[nodiscard]] StringView name() const override;
   [[nodiscard]] const ui_core::TextureRegion& icon() const override;
   [[nodiscard]] ui_core::IWidget& widget() override;
   [[nodiscard]] ResourceName asset_name() const override;

 private:
   State m_state;
   desktop_ui::RadioGroup m_tool_radio_group;
   renderer::Scene m_scene;
   renderer::BindlessScene m_bindless_scene;
   renderer::Config m_config;
   renderer::UpdateViewParamsJob m_update_view_params_job;
   renderer::OcclusionCulling m_occlusion_culling;
   renderer::RenderingJob m_rendering_job;
   const renderer::SceneObject* m_selected_object{};
   renderer::ObjectID m_selected_object_id{renderer::UNSELECTED_OBJECT};
   LevelViewport* m_viewport{};
   SelectionTool m_selection_tool;
   TranslationTool m_translation_tool;
   RotationTool m_rotation_tool;
   ScalingTool m_scaling_tool;
   LevelEditorSidePanel* m_side_panel;
   desktop_ui::DropDownMenu* m_origin_selector;
   desktop_ui::DropDownMenu* m_snap_selector;
   desktop_ui::DropDownMenu* m_speed_selector;
   HistoryManager m_history_manager;

   ILevelEditorTool* m_current_tool = nullptr;

   TG_SINK(desktop_ui::RadioGroup, OnSelection);
   TG_OPT_SINK(desktop_ui::DropDownMenu, OnSelected);
};

}// namespace triglav::editor
