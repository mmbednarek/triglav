#pragma once

#include "detail/Crc.hpp"
#include "ResourceType.hpp"

#include <cassert>
#include <cstdint>
#include <string_view>

namespace triglav {

using NameID = std::uint64_t;

class Name;

template<ResourceType CResourceType>
class TypedName
{
   friend Name;

 public:
   static constexpr auto resource_type = CResourceType;

   constexpr explicit TypedName(const NameID name) :
       m_name(name)
   {
   }

   // ReSharper disable once CppNonExplicitConversionOperator
   [[nodiscard]] operator Name() const;// NOLINT(google-explicit-constructor)

   constexpr auto operator<=>(const TypedName &other) const = default;
   constexpr bool operator==(const TypedName &other) const  = default;
   constexpr bool operator!=(const TypedName &other) const  = default;

 private:
   NameID m_name;
};

class Name
{
 public:
   constexpr Name() :
       m_type(ResourceType::Unknown),
       m_name(0)
   {
   }

   constexpr Name(const ResourceType type, const NameID nameID) :
       m_type(type),
       m_name(nameID)
   {
   }

   // ReSharper disable once CppNonExplicitConversionOperator
   template<ResourceType CResourceType>
   [[nodiscard]] constexpr operator TypedName<CResourceType>() const// NOLINT(google-explicit-constructor)
   {
      assert(CResourceType == m_type);
      return TypedName<CResourceType>(m_name);
   }

   [[nodiscard]] ResourceType type() const
   {
      return m_type;
   }

   constexpr auto operator<=>(const Name &other) const
   {
      if (m_name == other.m_name && m_type != other.m_type)
         return m_type <=> other.m_type;
      return m_name <=> other.m_name;
   }

   constexpr bool operator==(const Name &other) const = default;
   constexpr bool operator!=(const Name &other) const = default;

 private:
   ResourceType m_type;
   NameID m_name;
};

template<ResourceType CResourceType>
TypedName<CResourceType>::operator Name() const
{
   return Name{CResourceType, m_name};
}

constexpr auto make_name_id(const std::string_view value)
{
   return detail::hash_string(value);
}

constexpr auto make_name(const std::string_view value)
{
   const auto at        = value.find_last_of('.');
   const auto extension = value.substr(at + 1);
   const auto hash      = detail::hash_string(value.substr(0, at));
   return Name(type_by_extension(extension), hash);
}

#define TG_RESOURCE_TYPE(name, ext, cppType) using name##Name = TypedName<ResourceType::name>;

TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

namespace name_literals {

constexpr Name operator""_name(const char *value, const std::size_t count)
{
   return make_name(std::string_view(value, count));
}

constexpr NameID operator""_name_id(const char *value, const std::size_t count)
{
   return detail::hash_string(std::string_view(value, count));
}

}// namespace name_literals

}// namespace triglav
