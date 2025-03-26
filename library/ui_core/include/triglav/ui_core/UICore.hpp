#pragma once

namespace triglav::ui_core {

enum class HorizontalAlignment
{
   Left,
   Center,
   Right,
};

constexpr float calculate_alignment(const HorizontalAlignment alignment, const float containerWidth, const float itemWidth)
{
   switch (alignment) {
   case HorizontalAlignment::Left:
      return 0.0f;
   case HorizontalAlignment::Center:
      return containerWidth * 0.5f - itemWidth * 0.5f;
   case HorizontalAlignment::Right:
      return containerWidth - itemWidth;
   }
   return 0.0f;
}

enum class VerticalAlignment
{
   Top,
   Center,
   Bottom,
};

constexpr float calculate_alignment(const VerticalAlignment alignment, const float containerHeight, const float itemHeight)
{
   switch (alignment) {
   case VerticalAlignment::Top:
      return 0.0f;
   case VerticalAlignment::Center:
      return containerHeight * 0.5f - itemHeight * 0.5f;
   case VerticalAlignment::Bottom:
      return containerHeight - itemHeight;
   }
   return 0.0f;
}

}// namespace triglav::ui_core
