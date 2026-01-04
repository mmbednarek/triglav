#pragma once

#include "MenuController.hpp"

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/PrimitiveHelpers.hpp"

#include <optional>

namespace triglav::desktop_ui {

class Dialog;
class DesktopUIManager;

class ITabWidget
{
 public:
   virtual ~ITabWidget() = default;

   [[nodiscard]] virtual StringView name() const = 0;
   [[nodiscard]] virtual const ui_core::TextureRegion& icon() const = 0;
   [[nodiscard]] virtual ui_core::IWidget& widget() = 0;
};

using ITabWidgetPtr = std::unique_ptr<ITabWidget>;

class BasicTabWidget final : public ITabWidget
{
 public:
   BasicTabWidget(const String& name, const ui_core::TextureRegion& icon, std::unique_ptr<ui_core::IWidget> widget);

   [[nodiscard]] StringView name() const override;
   [[nodiscard]] const ui_core::TextureRegion& icon() const override;
   [[nodiscard]] ui_core::IWidget& widget() override;

 private:
   String m_name;
   ui_core::TextureRegion m_icon;
   std::unique_ptr<ui_core::IWidget> m_widget;
};

class TabView final : public ui_core::BaseWidget
{
 public:
   TG_EVENT(OnChangedActiveTab, u32 /*index*/, IWidget* /*widget*/)

   struct State
   {
      DesktopUIManager* manager;
      u32 active_tab = 0;
      bool allow_closing = true;
   };

   struct Measure
   {
      Vector2 size;
      std::vector<float> tab_widths;
      float tab_height;
   };

   TabView(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 available_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   bool on_mouse_moved(const ui_core::Event&);
   bool on_mouse_pressed(const ui_core::Event&, const ui_core::Event::Mouse&);
   bool on_mouse_released(const ui_core::Event&, const ui_core::Event::Mouse&);

   void set_active_tab(u32 active_tab);
   void remove_tab(u32 tab_id);
   [[nodiscard]] Vector4 content_area() const;

   u32 add_tab(ITabWidgetPtr&& widget);
   void set_default_widget(ui_core::IWidgetPtr&& widget);

   template<typename TChild, typename... TArgs>
   TChild& emplace_tab(TArgs&&... args)
   {
      return dynamic_cast<TChild&>(this->add_tab(std::make_unique<TChild>(std::forward<TArgs>(args)...)));
   }

 private:
   [[nodiscard]] std::pair<float, u32> index_from_mouse_position(Vector2 position) const;
   [[nodiscard]] const Measure& get_measure(Vector2 available_size) const;

   ui_core::Context& m_context;
   State m_state;
   std::vector<ITabWidgetPtr> m_tabs;
   std::vector<ui_core::SpriteInstance> m_icons;
   std::vector<ui_core::TextInstance> m_labels;
   ui_core::IWidgetPtr m_default_widget;
   std::map<float, u32> m_offset_to_item;
   Vector4 m_dimensions;
   Vector4 m_cropping_mask;

   ui_core::RectInstance m_background{};
   ui_core::RectInstance m_hover_rect{};
   ui_core::RectInstance m_active_rect{};
   ui_core::SpriteInstance m_close_button;
   u32 m_hovered_item = 0;
   bool m_is_dragging{false};
   bool m_is_widget_added = false;

   mutable Vector2 m_cached_measure_size{};
   mutable std::optional<Measure> m_cached_measure{};
};

}// namespace triglav::desktop_ui
