#pragma once

#include <span>
#include <utility>
#include <variant>

namespace triglav {

template<typename TClass>
struct ClassContainer
{
   using Type = TClass;
};

template<typename TMemberType, typename TClass>
constexpr ClassContainer<TClass> get_class_of_member(TMemberType TClass::* /*member*/)
{
   return ClassContainer<TClass>{};
}

template<typename TMemberType, typename TClass>
constexpr ClassContainer<TMemberType> get_type_of_member(TMemberType TClass::* /*member*/)
{
   return ClassContainer<TMemberType>{};
}

template<auto CMember>
using ClassOfMember = typename decltype(get_class_of_member(CMember))::Type;
template<auto CMember>
using TypeOfMember = typename decltype(get_type_of_member(CMember))::Type;

namespace detail {

template<typename T, size_t... Indices>
std::array<T, sizeof...(Indices)> span_to_array_indices(const std::span<T> jobFrames)
{
   return std::array<T, sizeof...(Indices)>{std::forward<T>(jobFrames[Indices])...};
}

template<typename T, size_t Count, size_t Last, size_t... Indices>
std::array<T, Count> span_to_array_internal(const std::span<T> jobFrames)
{
   if constexpr (sizeof...(Indices) == (Count - 1)) {
      return span_to_array_indices<T, Indices..., Last>(jobFrames);
   } else {
      return span_to_array_internal<T, Count, Last + 1, Indices..., Last>(jobFrames);
   }
}

}// namespace detail

template<typename T, size_t Count>
std::array<T, Count> span_to_array(const std::span<T> jobFrames)
{
   return detail::span_to_array_internal<T, Count, 0>(jobFrames);
}

template<typename T>
using NonVoid = std::conditional_t<std::is_same_v<T, void>, std::monostate, T>;

}// namespace triglav