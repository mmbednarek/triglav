#pragma once

namespace triglav {

template <typename TEnum>
struct EnumFlags
{
   using UnderlayingType = std::underlying_type_t<TEnum>;
   using EnumType = TEnum;

   UnderlayingType value;

   constexpr EnumFlags(const UnderlayingType value) :
       value(value)
   {
   }

   constexpr EnumFlags(const TEnum value) :
       value(static_cast<UnderlayingType>(value))
   {
   }

   constexpr bool operator&(const TEnum rhs) const
   {
      return (this->value & static_cast<UnderlayingType>(rhs)) != 0;
   }

   constexpr bool operator&(const EnumFlags rhs) const
   {
      return (this->value & rhs.value) == rhs.value;
   }

   constexpr EnumFlags operator|(TEnum rhs) const
   {
      return EnumFlags{this->value | static_cast<UnderlayingType>(rhs)};
   }

   constexpr EnumFlags operator|(const EnumFlags rhs) const
   {
      return EnumFlags{this->value | rhs.value};
   }

   constexpr EnumFlags& operator|=(TEnum rhs)
   {
      this->value |= static_cast<UnderlayingType>(rhs);
      return *this;
   }

   constexpr bool operator==(const EnumFlags rhs)
   {
      return this->value == rhs.value;
   }
};

}

#define TRIGLAV_DECL_FLAGS(enum_name) using enum_name##Flags = ::triglav::EnumFlags<enum_name>; \
   constexpr enum_name##Flags operator|(const enum_name lhs, const enum_name rhs) \
   { \
      return enum_name##Flags{std::to_underlying(lhs) | std::to_underlying(rhs)}; \
   }
