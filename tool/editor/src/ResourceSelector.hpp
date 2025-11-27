#pragma once

#include "triglav/desktop_ui/DesktopUI.hpp"
#include "triglav/desktop_ui/Dialog.hpp"
#include "triglav/desktop_ui/TextInput.hpp"
#include "triglav/event/Delegate.hpp"
#include "triglav/ui_core/IWidget.hpp"

namespace triglav::editor {

class ResourceList;
class ResourceSelectTrigger;

class ResourceSelector : public ui_core::ProxyWidget
{
 public:
   using Self = ResourceSelector;

   struct State
   {
      desktop_ui::DesktopUIManager* manager;
      ResourceSelectTrigger* trigger;
   };

   ResourceSelector(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   void on_typing(StringView text) const;

 private:
   State m_state;
   ResourceList* m_resource_list;
   TG_OPT_SINK(desktop_ui::TextInput, OnTyping);
};

class ResourceSelectTrigger final : public ui_core::ProxyWidget
{
 public:
   using Self = ResourceSelector;

   TG_EVENT(OnSelected, String)

   struct State
   {
      desktop_ui::DesktopUIManager* manager;
      String suffix;
   };
   ResourceSelectTrigger(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void on_event(const ui_core::Event& event) override;
   [[nodiscard]] const State& state() const;

   void dispatch_select(String result);

 private:
   State m_state;
   std::unique_ptr<desktop_ui::DesktopUIManager> m_desktop_manager;
   desktop_ui::Dialog* m_dialog = nullptr;
   Vector4 m_dimensions{};
};

}// namespace triglav::editor
