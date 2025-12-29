#include "ResourceSelector.hpp"

#include "triglav/Ranges.hpp"
#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/PopupManager.hpp"
#include "triglav/desktop_ui/SecondaryEventGenerator.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/render_core/GlyphCache.hpp"
#include "triglav/resource/ResourceManager.hpp"
#include "triglav/ui_core/Context.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/PrimitiveHelpers.hpp"
#include "triglav/ui_core/widget/RectBox.hpp"
#include "triglav/ui_core/widget/ScrollBox.hpp"
#include "triglav/ui_core/widget/VerticalLayout.hpp"

#include <vector>

namespace triglav::editor {

namespace {

constexpr auto g_vertical_margin = 5.0f;
constexpr auto g_horizontal_margin = 10.0f;

}// namespace

using namespace string_literals;

class ResourceList final : public ui_core::BaseWidget
{
 public:
   struct State
   {
      desktop_ui::DesktopUIManager* manager;
      ResourceSelectTrigger* trigger;
   };

   ResourceList(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
       ui_core::BaseWidget(parent),
       m_context(ctx),
       m_state(state),
       m_selection{
          .color = TG_THEME_VAL(active_color),
       }
   {
      this->populate_text_instances(StringView{""});
   }

   [[nodiscard]] Vector2 desired_size(const Vector2 available_size) const override
   {
      const auto& atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), TG_THEME_VAL(base_font_size)});
      const auto measure = atlas.measure_text("0"_strv);
      return {available_size.x, (measure.height + 2 * g_vertical_margin) * static_cast<float>(m_text_instances.size())};
   }

   void add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask) override
   {
      const auto& atlas = m_context.glyph_cache().find_glyph_atlas({TG_THEME_VAL(base_typeface), TG_THEME_VAL(base_font_size)});
      const auto measure = atlas.measure_text("0"_strv);

      m_dimensions = dimensions;
      m_cropping_mask = cropping_mask;

      float offset_y = 0.0f;
      for (auto [index, instance] : Enumerate(m_text_instances)) {
         const auto position = rect_position(dimensions) + Vector2(0, offset_y);
         const Vector4 rect{position, dimensions.z, measure.height + 2 * g_vertical_margin};
         m_offsets[offset_y + rect.w] = static_cast<u32>(index);

         if (index == m_selection_index) {
            m_selection.add(m_context, rect, cropping_mask);
         }

         if (do_regions_intersect(rect, cropping_mask)) {
            instance.add(m_context, position + Vector2(g_horizontal_margin, measure.height + g_vertical_margin), cropping_mask);
         } else {
            instance.remove(m_context);
         }
         offset_y += measure.height + 2 * g_vertical_margin;
      }
   }

   void remove_from_viewport() override
   {
      for (auto& instance : m_text_instances) {
         instance.remove(m_context);
      }
   }

   void filter(const StringView pattern)
   {
      this->remove_from_viewport();
      m_text_instances.clear();
      this->populate_text_instances(pattern);
      m_selection_index = 0;
      this->add_to_viewport(m_dimensions, m_cropping_mask);
   }

   void populate_text_instances(const StringView pattern)
   {
      m_context.resource_manager().name_registry().iterate_names([&](const std::string& name) {
         if (name.contains(pattern.to_std()) && name.ends_with(m_state.trigger->state().suffix.view().to_std())) {
            m_text_instances.push_back({
               .content = String{name.data(), name.size()},
               .typeface_name = TG_THEME_VAL(base_typeface),
               .font_size = TG_THEME_VAL(base_font_size),
               .color = TG_THEME_VAL(foreground_color),
            });
         }
      });
   }

   void on_event(const ui_core::Event& event) override
   {
      ui_core::visit_event<void>(*this, event);
   }

   void on_mouse_moved(const ui_core::Event& event)
   {
      const auto it = m_offsets.lower_bound(event.mouse_position.y);
      if (it == m_offsets.end()) {
         return;
      }

      m_selection_index = it->second;
      this->add_to_viewport(m_dimensions, m_cropping_mask);
   }

   [[nodiscard]] StringView selected_path() const
   {
      if (m_selection_index < m_text_instances.size()) {
         return m_text_instances[m_selection_index].content.view();
      }
      return {"", 0};
   }

   void on_mouse_pressed(const ui_core::Event& /*event*/, const ui_core::Event::Mouse& /*mouse*/) const
   {
      if (m_selection_index < m_text_instances.size()) {
         m_state.trigger->dispatch_select(m_text_instances[m_selection_index].content);
      }
   }

 private:
   ui_core::Context& m_context;
   State m_state;
   ui_core::RectInstance m_selection;
   std::vector<ui_core::TextInstance> m_text_instances;
   std::map<float, u32> m_offsets;
   Vector4 m_dimensions{};
   Vector4 m_cropping_mask{};
   u32 m_selection_index{};
};

ResourceSelector::ResourceSelector(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(ctx, parent),
    m_state(state)
{
   auto& background = this->create_content<ui_core::RectBox>({
      .color = TG_THEME_VAL(background_color_brighter),
      .border_radius = {},
      .border_color = TG_THEME_VAL(active_color),
      .border_width = 1.0f,
   });
   auto& layout = background.create_content<ui_core::VerticalLayout>({
      .padding = {5.0f, 5.0f, 5.0f, 5.0f},
      .separation = 5.0f,
   });

   auto& text_input = layout.create_child<desktop_ui::TextInput>({
      .manager = m_state.manager,
      .text = "",
   });
   TG_CONNECT_OPT(text_input, OnTyping, on_typing);
   TG_CONNECT_OPT(text_input, OnTextChanged, on_text_changed);

   m_resource_list = &layout
                         .create_child<ui_core::ScrollBox>({
                            .offset = 0.0f,
                         })
                         .create_content<ResourceList>({
                            .manager = m_state.manager,
                            .trigger = m_state.trigger,
                         });
}

void ResourceSelector::on_typing(const StringView text) const
{
   assert(m_resource_list != nullptr);
   m_resource_list->filter(text);
}

void ResourceSelector::on_text_changed(const StringView /*text*/) const
{
   m_state.trigger->dispatch_select(m_resource_list->selected_path());
}

ResourceSelectTrigger::ResourceSelectTrigger(ui_core::Context& ctx, State state, ui_core::IWidget* parent) :
    ui_core::ProxyWidget(ctx, parent),
    m_state(std::move(state))
{
}

void ResourceSelectTrigger::add_to_viewport(const Vector4 dimensions, const Vector4 cropping_mask)
{
   m_dimensions = dimensions;
   ProxyWidget::add_to_viewport(dimensions, cropping_mask);
}

void ResourceSelectTrigger::on_event(const ui_core::Event& event)
{
   if (event.event_type == ui_core::Event::Type::MousePressed) {
      auto& popup =
         m_state.manager->popup_manager().create_popup_dialog(rect_position(m_dimensions) + Vector2{0, m_dimensions.w}, {240, 400});
      auto& root_widget =
         popup.emplace_root_widget<desktop_ui::SecondaryEventGenerator>(popup.widget_renderer().context(), nullptr, popup.surface());

      m_desktop_manager =
         std::make_unique<desktop_ui::DesktopUIManager>(m_state.manager->properties(), popup.surface(), m_state.manager->popup_manager());
      root_widget.create_content<ResourceSelector>({
         .manager = m_desktop_manager.get(),
         .trigger = this,
      });
      popup.initialize();

      m_dialog = &popup;
   } else {
      ProxyWidget::on_event(event);
   }
}

const ResourceSelectTrigger::State& ResourceSelectTrigger::state() const
{
   return m_state;
}

void ResourceSelectTrigger::dispatch_select(String result)
{
   event_OnSelected.publish(std::move(result));
   if (m_dialog != nullptr) {
      m_state.manager->popup_manager().close_popup(m_dialog);
      m_dialog = nullptr;
   }
}

}// namespace triglav::editor
