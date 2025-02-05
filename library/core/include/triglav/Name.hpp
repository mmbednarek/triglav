#pragma once

#include "CompTimeString.hpp"
#include "ResourceType.hpp"
#include "detail/Crc.hpp"

#include <cassert>
#include <cstdint>
#include <string_view>

namespace triglav {

using Name = std::uint64_t;

class ResourceName;

template<ResourceType CResourceType>
class TypedName
{
   friend ResourceName;

 public:
   static constexpr auto resource_type = CResourceType;

   constexpr explicit TypedName(const Name name) :
       m_name(name)
   {
   }

   [[nodiscard]] constexpr Name name() const
   {
      return m_name;
   }

   // ReSharper disable once CppNonExplicitConversionOperator
   [[nodiscard]] operator ResourceName() const;// NOLINT(google-explicit-constructor)

   constexpr auto operator<=>(const TypedName& other) const = default;
   constexpr bool operator==(const TypedName& other) const = default;
   constexpr bool operator!=(const TypedName& other) const = default;

 private:
   Name m_name;
};

class ResourceName
{
 public:
   constexpr ResourceName() :
       m_type(ResourceType::Unknown),
       m_name(0)
   {
   }

   constexpr ResourceName(const ResourceType type, const Name nameID) :
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

   [[nodiscard]] constexpr int loading_stage() const
   {
      return g_resourceStage[static_cast<int>(m_type)];
   }

   [[nodiscard]] constexpr Name name() const
   {
      return m_name;
   }

   constexpr auto operator<=>(const ResourceName& other) const
   {
      if (m_name == other.m_name && m_type != other.m_type)
         return m_type <=> other.m_type;
      return m_name <=> other.m_name;
   }

   constexpr bool operator==(const ResourceName& other) const = default;
   constexpr bool operator!=(const ResourceName& other) const = default;

   template<ResourceType CResourceType>
   constexpr bool operator==(const TypedName<CResourceType>& other) const
   {
      return (*this) == static_cast<ResourceName>(other);
   }

 private:
   ResourceType m_type;
   Name m_name;
};

constexpr auto g_emptyResource = ResourceName{};

template<ResourceType CResourceType>
TypedName<CResourceType>::operator ResourceName() const
{
   return ResourceName{CResourceType, m_name};
}

constexpr auto make_name_id(const std::string_view value)
{
   return detail::hash_string(value);
}

template<size_t CSize>
constexpr auto comptime_str(const char (&value)[CSize])
{
   return std::string_view{value, CSize};
}

template<CompTimeString TValue>
constexpr auto make_resource_path_comptime()
{
   constexpr auto value = TValue.to_string_view();
   constexpr auto at = value.find_last_of('.');
   constexpr auto extension = value.substr(at + 1);
   constexpr auto hash = detail::hash_string(value.substr(0, at));
   return TypedName<type_by_extension(extension)>(hash);
}

constexpr auto make_rc_name(const std::string_view value)
{
   const auto at = value.find_last_of('.');
   const auto extension = value.substr(at + 1);
   const auto hash = detail::hash_string(value.substr(0, at));
   return ResourceName(type_by_extension(extension), hash);
}

#define TG_RESOURCE_TYPE(name, ext, cppType, stage) using name##Name = TypedName<ResourceType::name>;

TG_RESOURCE_TYPE_LIST

#undef TG_RESOURCE_TYPE

namespace name_literals {

template<CompTimeString TValue>
constexpr auto operator""_rc()
{
   return make_resource_path_comptime<TValue>();
}

constexpr Name operator""_name(const char* value, const std::size_t count)
{
   return detail::hash_string(std::string_view(value, count));
}

}// namespace name_literals

}// namespace triglav
