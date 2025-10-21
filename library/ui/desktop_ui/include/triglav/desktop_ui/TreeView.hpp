#pragma once

#include "TreeController.hpp"

#include "triglav/ui_core/IWidget.hpp"
#include "triglav/ui_core/Primitives.hpp"

#include <optional>
#include <set>

namespace triglav::desktop_ui {

class DesktopUIManager;

class TreeView : public ui_core::BaseWidget
{
 public:
   using Self = TreeView;
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

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const ui_core::Event& event) override;

   void on_mouse_pressed(const ui_core::Event&, const ui_core::Event::Mouse&);


 private:
   [[nodiscard]] std::pair<float, TreeItemId> index_from_mouse_position(Vector2 position) const;
   [[nodiscard]] Measure get_measure(TreeItemId parentId = TREE_ROOT) const;
   void draw_level(TreeItemId parentId, float offset_x, float& offset_y);
   void remove_level(TreeItemId parentId);

   ui_core::Context& m_context;
   State m_state;
   Vector4 m_dimensions{};
   Vector4 m_croppingMask{};
   std::map<TreeItemId, ui_core::TextId> m_labels;
   std::map<TreeItemId, ui_core::SpriteId> m_icons;
   std::map<TreeItemId, ui_core::SpriteId> m_arrows;
   std::map<float, TreeItemId> m_offsetToItemId;
   ui_core::RectId m_itemHighlight{};
   Vector4 m_highlightDims{};

   mutable std::optional<Measure> m_cachedMeasure{};
};

}// namespace triglav::desktop_ui
