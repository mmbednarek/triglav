#include "LevelEditor.hpp"

#include "../RootWindow.hpp"
#include "LevelEditorSidePanel.hpp"
#include "LevelViewport.hpp"

#include "triglav/desktop_ui/CheckBox.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/Splitter.hpp"
#include "triglav/renderer/stage/AmbientOcclusionStage.hpp"
#include "triglav/renderer/stage/GBufferStage.hpp"
#include "triglav/renderer/stage/ShadingStage.hpp"
#include "triglav/renderer/stage/ShadowMapStage.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/widget/AlignmentBox.hpp"
#include "triglav/ui_core/widget/EmptySpace.hpp"
#include "triglav/ui_core/widget/HorizontalLayout.hpp"
#include "triglav/ui_core/widget/Image.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/SizeLimit.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

namespace triglav::editor {

using namespace name_literals;

namespace {

constexpr Vector2 ICON_SIZE{22.0f, 22.0f};
constexpr float TOOLBAR_HEIGHT = 48.0f;

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
    m_scalingTool(*this),
    m_currentTool(&m_selectionTool),
    TG_CONNECT(m_toolRadioGroup, OnSelection, on_selected_tool)
{
   auto& splitter = this->create_content<desktop_ui::Splitter>({
      .manager = m_state.manager,
      .offset = 260,
      .axis = ui_core::Axis::Horizontal,
      .offset_type = desktop_ui::SplitterOffsetType::Following,
   });

   m_sidePanel = &splitter.create_following<LevelEditorSidePanel>({
      .manager = m_state.manager,
      .editor = this,
   });

   auto& left_layout = splitter.create_preceding<ui_core::VerticalLayout>({});
   auto& toolbar = left_layout.create_child<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_brighter),
      .borderRadius = {0, 0, 0, 0},
      .borderColor = palette::NO_COLOR,
      .borderWidth = 0.0f,
   });

   auto& toolbar_layout = toolbar
                             .create_content<ui_core::SizeLimit>({
                                .max_size = {4096, TOOLBAR_HEIGHT},
                             })
                             .create_content<ui_core::AlignmentBox>({
                                .horizontalAlignment = std::nullopt,
                                .verticalAlignment = ui_core::VerticalAlignment::Center,
                             })
                             .create_content<ui_core::HorizontalLayout>({
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

   // Separation
   toolbar_layout.create_child<ui_core::EmptySpace>({{5.0f, 0.0f}});

   toolbar_layout
      .create_child<ui_core::RectBox>({
         .color = {0.2f, 0.2f, 0.2f, 1.0f},
      })
      .create_content<ui_core::EmptySpace>({
         .size = {2.0f, ICON_SIZE.y},
      });

   toolbar_layout.create_child<ui_core::EmptySpace>({{5.0f, 0.0f}});

   toolbar_layout
      .create_child<ui_core::SizeLimit>({
         .max_size = {ICON_SIZE.x, ICON_SIZE.y + 4},
      })
      .create_content<ui_core::AlignmentBox>({
         .horizontalAlignment = std::nullopt,
         .verticalAlignment = ui_core::VerticalAlignment::Center,
      })
      .create_content<ui_core::Image>({
         .texture = "texture/ui_atlas.tex"_rc,
         .maxSize = ICON_SIZE,
         .region = Vector4{4 * 64, 0, 64, 64},
      });

   m_originSelector = &toolbar_layout.create_child<desktop_ui::DropDownMenu>({
      .manager = m_state.manager,
      .items = {"Origin", "Centroid", "World"},
      .selectedItem = 0,
   });
   TG_CONNECT_OPT(*m_originSelector, OnSelected, on_origin_selected);

   toolbar_layout.create_child<ui_core::EmptySpace>({{5.0f, 0.0f}});

   toolbar_layout
      .create_child<ui_core::SizeLimit>({
         .max_size = {ICON_SIZE.x, ICON_SIZE.y + 4},
      })
      .create_content<ui_core::AlignmentBox>({
         .horizontalAlignment = std::nullopt,
         .verticalAlignment = ui_core::VerticalAlignment::Center,
      })
      .create_content<ui_core::Image>({
         .texture = "texture/ui_atlas.tex"_rc,
         .maxSize = ICON_SIZE,
         .region = Vector4{4 * 64, 64, 64, 64},
      });

   m_snapSelector = &toolbar_layout.create_child<desktop_ui::DropDownMenu>({
      .manager = m_state.manager,
      .items = {"Off", "0.25", "0.5", "1", "2", "4"},
      .selectedItem = 0,
   });

   toolbar_layout.create_child<ui_core::EmptySpace>({{5.0f, 0.0f}});

   toolbar_layout
      .create_child<ui_core::SizeLimit>({
         .max_size = {ICON_SIZE.x, ICON_SIZE.y + 4},
      })
      .create_content<ui_core::AlignmentBox>({
         .horizontalAlignment = std::nullopt,
         .verticalAlignment = ui_core::VerticalAlignment::Center,
      })
      .create_content<ui_core::Image>({
         .texture = "texture/ui_atlas.tex"_rc,
         .maxSize = ICON_SIZE,
         .region = Vector4{4 * 64, 2 * 64, 64, 64},
      });

   m_speedSelector = &toolbar_layout.create_child<desktop_ui::DropDownMenu>({
      .manager = m_state.manager,
      .items = {"8.0", "16.0", "32.0", "64.0", "128.0"},
      .selectedItem = 1,
   });

   m_viewport = &left_layout.emplace_child<LevelViewport>(&left_layout, *m_state.rootWindow, *this);

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

Vector3 LevelEditor::selected_object_position(const std::optional<Vector3> position) const
{
   const auto* object = this->selected_object();
   const auto translation = position.value_or(object->transform.translation);

   switch (m_originSelector->selected_item()) {
   case 0:
      return translation;
   case 1: {
      const auto& mesh = this->root_window().resource_manager().get(object->model);
      const auto centroid = mesh.boundingBox.centroid();
      return translation + centroid * object->transform.scale;
   }
   default:
      break;
   }
   return {0, 0, 0};
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
   case 3:
      m_currentTool = &m_scalingTool;
      break;
   default:
      break;
   }
   m_viewport->update_view();
}

void LevelEditor::on_origin_selected(u32 /*id*/) const
{
   m_viewport->update_view();
}

float LevelEditor::snap_offset(const float offset) const
{
   static constexpr float values[] = {0.0f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};

   if (m_snapSelector->selected_item() == 0)
      return offset;

   const float step = values[m_snapSelector->selected_item()];
   return std::round(offset / step) * step;
}

Vector3 LevelEditor::snap_offset(const Vector3 offset) const
{
   if (m_snapSelector->selected_item() == 0)
      return offset;

   const auto length = glm::length(offset);
   const auto norm = offset / length;

   return snap_offset(length) * norm;
}

float LevelEditor::speed() const
{
   static constexpr float values[] = {8.0f, 16.0f, 32.0f, 64.0f, 128.0f};
   return values[m_speedSelector->selected_item()];
}

void LevelEditor::finish_using_tool() const
{
   if (m_selectedObject != nullptr) {
      m_sidePanel->on_changed_selected_object(*m_selectedObject);
   }
}
void LevelEditor::set_selected_transform(const Transform3D& transform)
{
   if (m_selectedObjectID == renderer::UNSELECTED_OBJECT)
      return;

   m_scene.set_transform(m_selectedObjectID, transform);
   m_sidePanel->on_changed_selected_object(*m_selectedObject);
}

void LevelEditor::set_selected_object(const renderer::ObjectID id)
{
   m_selectedObjectID = id;

   if (id == renderer::UNSELECTED_OBJECT) {
      m_selectedObject = nullptr;
   } else {
      m_selectedObject = &scene().object(id);
      m_sidePanel->on_changed_selected_object(*m_selectedObject);
   }
}

HistoryManager& LevelEditor::history_manager()
{
   return m_historyManager;
}

}// namespace triglav::editor
