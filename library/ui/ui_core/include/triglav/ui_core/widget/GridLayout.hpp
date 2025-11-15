#pragma once

#include "../IWidget.hpp"

#include "triglav/Logging.hpp"

#include <vector>

namespace triglav::ui_core {

class Context;

class GridLayout final : public LayoutWidget
{
   TG_DEFINE_LOG_CATEGORY(GridLayout)
 public:
   struct State
   {
      std::vector<float> column_ratios;
      std::vector<float> row_ratios;
      float horizontal_spacing;
      float vertical_spacing;
   };

   GridLayout(Context& context, State state, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;
   void on_event(const Event& event) override;

 private:
   std::optional<MemorySize> find_active_child() const;

   State m_state;
   Vector4 m_dimensions;
   u32 m_lastRow = 0;
   u32 m_lastCol = 0;
   std::optional<MemorySize> m_activeChild{};
};

}// namespace triglav::ui_core