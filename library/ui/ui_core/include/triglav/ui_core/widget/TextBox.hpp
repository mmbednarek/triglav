#pragma once

#include "../IWidget.hpp"
#include "../Primitives.hpp"
#include "../UICore.hpp"

#include "triglav/Name.hpp"

#include <optional>
#include <string>

namespace triglav::ui_core {

class Context;

class TextBox final : public IWidget
{
 public:
   struct State
   {
      i32 fontSize{};
      TypefaceName typeface{};
      String content{};
      Vector4 color{};
      HorizontalAlignment horizontalAlignment{};
      VerticalAlignment verticalAlignment{};
   };

   TextBox(Context& ctx, State initialState, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions, Vector4 croppingMask) override;
   void remove_from_viewport() override;

   void set_content(StringView content);
   void set_color(Vector4 color);
   const State& state() const;

 private:
   [[nodiscard]] Vector2 calculate_desired_size() const;

   Context& m_uiContext;
   State m_state{};
   IWidget* m_parent;
   TextId m_id{};
   mutable std::optional<Vector2> m_cachedDesiredSize{};
};

}// namespace triglav::ui_core