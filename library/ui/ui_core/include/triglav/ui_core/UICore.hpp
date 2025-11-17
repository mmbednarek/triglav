#pragma once

namespace triglav::ui_core {

enum class Axis
{
   Horizontal,
   Vertical,
};

constexpr float& parallel(Vector2& vec, const Axis axis)
{
   switch (axis) {
   case Axis::Horizontal:
      return vec.x;
   case Axis::Vertical:
      return vec.y;
   }
   return vec.x;
}

constexpr float parallel(const Vector2& vec, const Axis axis)
{
   switch (axis) {
   case Axis::Horizontal:
      return vec.x;
   case Axis::Vertical:
      return vec.y;
   }
   return 0.0f;
}

constexpr float& ortho(Vector2& vec, const Axis axis)
{
   switch (axis) {
   case Axis::Horizontal:
      return vec.y;
   case Axis::Vertical:
      return vec.x;
   }
   return vec.x;
}

constexpr float ortho(const Vector2& vec, const Axis axis)
{
   switch (axis) {
   case Axis::Horizontal:
      return vec.y;
   case Axis::Vertical:
      return vec.x;
   }
   return 0.0f;
}

constexpr Vector2 make_vec(const Axis axis, const float parallel, const float orthogonal)
{
   switch (axis) {
   case Axis::Horizontal:
      return {parallel, orthogonal};
   case Axis::Vertical:
      return {orthogonal, parallel};
   }
   return {};
}

#define TG_DECLARE_AXIS_FUNCS(axis_member)                                                 \
   [[nodiscard]] constexpr float& parallel(Vector2& vec) const                             \
   {                                                                                       \
      return ui_core::parallel(vec, axis_member);                                          \
   }                                                                                       \
   [[nodiscard]] constexpr float parallel(const Vector2& vec) const                        \
   {                                                                                       \
      return ui_core::parallel(vec, axis_member);                                          \
   }                                                                                       \
   [[nodiscard]] constexpr float& ortho(Vector2& vec) const                                \
   {                                                                                       \
      return ui_core::ortho(vec, axis_member);                                             \
   }                                                                                       \
   [[nodiscard]] constexpr float ortho(const Vector2& vec) const                           \
   {                                                                                       \
      return ui_core::ortho(vec, axis_member);                                             \
   }                                                                                       \
   [[nodiscard]] constexpr Vector2 make_vec(const float parallel, const float ortho) const \
   {                                                                                       \
      return ui_core::make_vec(axis_member, parallel, ortho);                              \
   }


enum class HorizontalAlignment
{
   Left,
   Center,
   Right,
};

constexpr float calculate_alignment(const HorizontalAlignment alignment, const float container_width, const float item_width)
{
   switch (alignment) {
   case HorizontalAlignment::Left:
      return 0.0f;
   case HorizontalAlignment::Center:
      return container_width * 0.5f - item_width * 0.5f;
   case HorizontalAlignment::Right:
      return container_width - item_width;
   }
   return 0.0f;
}

enum class VerticalAlignment
{
   Top,
   Center,
   Bottom,
};

constexpr float calculate_alignment(const VerticalAlignment alignment, const float container_height, const float item_height)
{
   switch (alignment) {
   case VerticalAlignment::Top:
      return 0.0f;
   case VerticalAlignment::Center:
      return container_height * 0.5f - item_height * 0.5f;
   case VerticalAlignment::Bottom:
      return container_height - item_height;
   }
   return 0.0f;
}

}// namespace triglav::ui_core
