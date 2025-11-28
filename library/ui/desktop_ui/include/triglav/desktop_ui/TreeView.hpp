#pragma once

#include "TreeController.hpp"
#include "triglav/event/Delegate.hpp"

#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/PrimitiveHelpers.hpp"
#include "triglav/ui_core/Primitives.hpp"

#include <optional>
#include <set>

namespace triglav::desktop_ui {

class DesktopUIManager;

class TreeView final : public ui_core::BaseWidget
{
 public:
   using Self = TreeView;

   TG_EVENT(OnSelected, TreeItemId)

   struct State
   {
      DesktopUIManager* manager;
      ITreeController* controller;
      std::set<TreeItemId> extended_items;
   };

   struct Measure
   {
      Vector2 size;
      Vector2 item_size;
      u32 total_count;
   };

   TreeView(ui_core::Context& ctx, State state, ui_core::IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parent_size) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 cropping_mask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;
   void set_selected_item(TreeItemId item_id);
   void remove_item(TreeItemId item_id);
   void set_label(TreeItemId item_id, StringView label);

   void on_mouse_pressed(const ui_core::Event&, const ui_core::Event::Mouse&);

 private:
   void remove_internal(const TreeItemId item_id);

   [[nodiscard]] std::pair<float, TreeItemId> index_from_mouse_position(Vector2 position) const;
   [[nodiscard]] Measure get_measure(TreeItemId parent_id = TREE_ROOT) const;
   void draw_level(TreeItemId parent_id, float offset_x, float& offset_y);
   void remove_level(TreeItemId parent_id);

   ui_core::Context& m_context;
   State m_state;
   Vector4 m_dimensions{};
   Vector4 m_cropping_mask{};
   std::map<TreeItemId, ui_core::TextInstance> m_labels;
   std::map<TreeItemId, ui_core::SpriteInstance> m_icons;
   std::map<TreeItemId, ui_core::SpriteInstance> m_arrows;
   std::map<float, TreeItemId> m_offset_to_item_id;
   ui_core::RectInstance m_item_highlight{};
   Vector4 m_highlight_dims{};
   TreeItemId m_selected_item = TREE_ROOT;

   mutable std::optional<Measure> m_cached_measure{};
};

}// namespace triglav::desktop_ui
