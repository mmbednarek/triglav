#include "LevelEditor.hpp"

#include "../RootWindow.hpp"
#include "LevelViewport.hpp"

#include "triglav/desktop_ui/CheckBox.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/renderer/stage/AmbientOcclusionStage.hpp"
#include "triglav/renderer/stage/GBufferStage.hpp"
#include "triglav/renderer/stage/ShadingStage.hpp"
#include "triglav/renderer/stage/ShadowMapStage.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::editor {

using namespace name_literals;

namespace {

constexpr Vector2 ICON_SIZE{20.0f, 20.0f};

renderer::Config get_default_config()
{
   renderer::Config result{};
   result.ambientOcclusion = renderer::AmbientOcclusionMethod::ScreenSpace;
   result.antialiasing = renderer::AntialiasingMethod::FastApproximate;
   result.shadowCasting = renderer::ShadowCastingMethod::ShadowMap;
   result.isBloomEnabled = true;
   result.isUIHidden = false;
   result.isSmoothCameraEnabled = true;
   result.isRenderingParticles = false;

   return result;
}

}// namespace

LevelEditor::LevelEditor(ui_core::Context& context, const State state, ui_core::IWidget* parent) :
    ProxyWidget(context, parent),
    m_state(state),
    m_scene(context.resource_manager()),
    m_bindlessScene(m_state.rootWindow->device(), context.resource_manager(), m_scene),
    m_config(get_default_config()),
    m_updateViewParamsJob(m_scene),
    m_occlusionCulling(m_updateViewParamsJob, m_bindlessScene),
    m_renderingJob(m_config),
    m_selectionTool(*this),
    m_translationTool(*this),
    m_rotationTool(*this),
    m_currentTool(&m_selectionTool),
    TG_CONNECT(m_toolRadioGroup, OnSelection, on_selected_tool)
{
   auto& layout = this->create_content<ui_core::VerticalLayout>({});
   auto& toolbar = layout.create_child<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_brighter),
      .borderRadius = {0, 0, 0, 0},
      .borderColor = palette::NO_COLOR,
      .borderWidth = 0.0f,
   });

   auto& toolbar_layout = toolbar.create_content<ui_core::HorizontalLayout>({
      .padding = {8.0f, 6.0f, 8.0f, 6.0f},
      .separation = 4.0f,
      .gravity = ui_core::HorizontalAlignment::Left,
   });

   auto& select_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   select_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = ICON_SIZE,
      .region = Vector4{192, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&select_btn);

   auto& move_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   move_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = ICON_SIZE,
      .region = Vector4{0, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&move_btn);

   auto& rotate_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   rotate_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = ICON_SIZE,
      .region = Vector4{64, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&rotate_btn);

   auto& scale_btn = toolbar_layout.create_child<desktop_ui::CheckBox>({
      .manager = m_state.manager,
      .radioGroup = &m_toolRadioGroup,
      .isEnabled = false,
   });
   scale_btn.create_content<ui_core::Image>({
      .texture = "texture/ui_atlas.tex"_rc,
      .maxSize = ICON_SIZE,
      .region = Vector4{128, 64, 64, 64},
   });
   m_toolRadioGroup.add_check_box(&scale_btn);

   m_viewport = &layout.emplace_child<LevelViewport>(&layout, *m_state.rootWindow, *this);

   m_scene.load_level("demo.level"_rc);
   m_bindlessScene.write_objects_to_buffer();
   m_scene.update_shadow_maps();

   m_renderingJob.emplace_stage<renderer::stage::GBufferStage>(m_state.rootWindow->device(), m_bindlessScene);
   m_renderingJob.emplace_stage<renderer::stage::AmbientOcclusionStage>(m_state.rootWindow->device());
   m_renderingJob.emplace_stage<renderer::stage::ShadowMapStage>(m_scene, m_bindlessScene, m_updateViewParamsJob);
   m_renderingJob.emplace_stage<renderer::stage::ShadingStage>();

   m_scene.set_camera({-20, -6, -5}, glm::quat(Vector3{0.13, 0.0, 5.29}));
}

[[nodiscard]] renderer::Scene& LevelEditor::scene()
{
   return m_scene;
}

void LevelEditor::tick(const float delta_time)
{
   m_bindlessScene.write_objects_to_buffer();
   if (m_viewport != nullptr) {
      m_viewport->tick(delta_time);
   }
}

const renderer::SceneObject* LevelEditor::selected_object() const
{
   return m_selectedObject;
}

renderer::ObjectID LevelEditor::selected_object_id() const
{
   return m_selectedObjectID;
}

LevelViewport& LevelEditor::viewport() const
{
   assert(m_viewport != nullptr);
   return *m_viewport;
}

ILevelEditorTool& LevelEditor::tool() const
{
   return *m_currentTool;
}

SelectionTool& LevelEditor::selection_tool()
{
   return m_selectionTool;
}

RootWindow& LevelEditor::root_window() const
{
   return *m_state.rootWindow;
}

void LevelEditor::on_selected_tool(const u32 id)
{
   m_currentTool->on_left_tool();
   switch (id) {
   case 0:
      m_currentTool = &m_selectionTool;
      break;
   case 1:
      m_currentTool = &m_translationTool;
      break;
   case 2:
      m_currentTool = &m_rotationTool;
      break;
   default:
      break;
   }
   m_viewport->update_view();
}

void LevelEditor::set_selected_object(const renderer::ObjectID id)
{
   m_selectedObject = &scene().object(id);
   m_selectedObjectID = id;
}

}// namespace triglav::editor
