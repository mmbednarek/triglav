#pragma once

#include "triglav/desktop_ui/CheckBox.hpp"
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

 private:
   State m_state;
   desktop_ui::RadioGroup m_toolRadioGroup;
   renderer::Scene m_scene;
   renderer::BindlessScene m_bindlessScene;
   renderer::Config m_config;
   renderer::UpdateViewParamsJob m_updateViewParamsJob;
   renderer::OcclusionCulling m_occlusionCulling;
   renderer::RenderingJob m_renderingJob;
   LevelViewport* m_viewport;
};

}// namespace triglav::editor
