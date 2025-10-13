#pragma once

#include <map>
#include <ranges>

#include "Int.hpp"

namespace triglav {

constexpr auto Range = std::ranges::views::iota;
constexpr auto Values = std::ranges::views::values;
constexpr auto Keys = std::ranges::views::keys;

#ifdef __cpp_lib_ranges_enumerate
constexpr auto Enumerate = std::ranges::views::enumerate;
#else

template<typename T>
concept HasConstIterator = requires { typename T::const_iterator; };

template<typename T>
concept HasElementType = requires { typename T::element_type; };

template<typename TRange>
struct ExtractIterator
{
   using iterator = typename TRange::iterator;
};

template<HasConstIterator TRange>
struct ExtractIterator<TRange>
{
   using iterator = std::conditional_t<std::is_const_v<TRange>, typename TRange::const_iterator, typename TRange::iterator>;
};

template<typename TRange>
struct ExtractElementType
{
   using type = typename TRange::value_type;
};

template<HasElementType TRange>
struct ExtractElementType<TRange>
{
   using type = typename TRange::element_type;
};

// dirty fallback for enumerate
template<typename TRange>
struct Enumerate
{
   using RangeIter = typename ExtractIterator<TRange>::iterator;
   using RangeValueBare = typename ExtractElementType<TRange>::type;
   using RangeValue = std::conditional_t<std::is_const_v<TRange>, const RangeValueBare, RangeValueBare>;

   struct EnumIterator
   {
      u32 counter;
      RangeIter it;

      std::tuple<u32, RangeValue&> operator*()
      {
         return {counter, *it};
      }

      EnumIterator& operator++()
      {
         ++counter;
         ++it;
         return *this;
      }

      bool operator!=(const EnumIterator& other)
      {
         return it != other.it;
      }
   };

   explicit Enumerate(TRange& range) :
       m_begin(range.begin()),
       m_end(range.end())
   {
   }

   EnumIterator begin()
   {
      return EnumIterator{0, m_begin};
   }

   EnumIterator end()
   {
      return EnumIterator{0, m_end};
   }

   RangeIter m_begin;
   RangeIter m_end;
};

#endif// __cpp_lib_ranges_enumerate

template<typename TIterator>
struct ItRange
{
   TIterator m_begin;
   TIterator m_end;

   TIterator begin() const
   {
      return m_begin;
   }

   TIterator end() const
   {
      return m_end;
   }
};

template<typename TMultiMap, typename TValue>
auto equal_range(TMultiMap& multi_map, TValue&& value)
{
   auto [beg, end] = multi_map.equal_range(std::forward<TValue>(value));
   return Values(ItRange<typename TMultiMap::iterator>{beg, end});
}

}// namespace triglav