#pragma once

#include "triglav/Name.hpp"
#include "triglav/ui_core/IWidget.hpp"

#include <optional>
#include <string>

namespace triglav::ui_core {

class Context;

enum class TextAlignment
{
   Left,
   Center,
   Right,
};

class TextBox final : public IWidget
{
 public:
   struct State
   {
      i32 fontSize{};
      TypefaceName typeface{};
      std::string content{};
      Vector4 color{};
      TextAlignment alignment{TextAlignment::Left};
   };

   TextBox(Context& ctx, State initialState, IWidget* parent);

   [[nodiscard]] Vector2 desired_size(Vector2 parentSize) const override;
   void add_to_viewport(Vector4 dimensions) override;
   void remove_from_viewport() override;

   void set_content(std::string_view content);

 private:
   [[nodiscard]] Vector2 calculate_desired_size() const;

   Context& m_uiContext;
   State m_state{};
   IWidget* m_parent;
   Name m_id{};
   mutable std::optional<Vector2> m_cachedDesiredSize{};
};

}// namespace triglav::ui_core