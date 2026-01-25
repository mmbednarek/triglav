#pragma once

#include <optional>
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
std::array<T, sizeof...(Indices)> span_to_array_indices(const std::span<T> job_frames)
{
   return std::array<T, sizeof...(Indices)>{std::forward<T>(job_frames[Indices])...};
}

template<typename T, size_t Count, size_t Last, size_t... Indices>
std::array<T, Count> span_to_array_internal(const std::span<T> job_frames)
{
   if constexpr (sizeof...(Indices) == (Count - 1)) {
      return span_to_array_indices<T, Indices..., Last>(job_frames);
   } else {
      return span_to_array_internal<T, Count, Last + 1, Indices..., Last>(job_frames);
   }
}

}// namespace detail

template<typename T, size_t Count>
std::array<T, Count> span_to_array(const std::span<T> job_frames)
{
   return detail::span_to_array_internal<T, Count, 0>(job_frames);
}

template<typename T>
using NonVoid = std::conditional_t<std::is_same_v<T, void>, std::monostate, T>;

template<typename T, typename TElement>
concept ForwardIterator = requires(T iterator) {
   { iterator.next() } -> std::same_as<std::optional<TElement>>;
};

template<typename TElem, ForwardIterator<TElem> TIterator>
class StlForwardIterator
{
 public:
   using iterator_category = std::forward_iterator_tag;
   using value_type = TElem;
   using difference_type = ptrdiff_t;
   using pointer = TElem*;
   using reference = TElem&;
   using Self = StlForwardIterator;

   StlForwardIterator(TIterator& iterator, std::optional<TElem> value) :
       m_iterator(iterator),
       m_value(std::move(value))
   {
   }

   Self& operator++()
   {
      m_value = m_iterator.next();
      return *this;
   }

   [[nodiscard]] TElem operator*() const
   {
      return *m_value;
   }

   [[nodiscard]] bool operator!=(const Self& other) const
   {
      return m_value.has_value() || other.m_value.has_value();
   }

 private:
   TIterator& m_iterator;
   std::optional<TElem> m_value{};
};

template<typename TElem, ForwardIterator<TElem> TIterator>
class StlForwardRange
{
 public:
   template<typename... TArgs>
   explicit StlForwardRange(TArgs... args) :
       m_iterator(std::forward<TArgs>(args)...)
   {
   }

   StlForwardIterator<TElem, TIterator> begin()
   {
      return StlForwardIterator<TElem, TIterator>{m_iterator, m_iterator.next()};
   }

   StlForwardIterator<TElem, TIterator> end()
   {
      return StlForwardIterator<TElem, TIterator>{m_iterator, std::nullopt};
   }

 private:
   TIterator m_iterator;
};

template<typename T, typename TElem>
concept HasPushBack = requires(T t, TElem elem) {
   { t.push_back(elem) };
};

template<typename TIt, typename TValue, typename TTrans>
TIt find_binary(TIt begin, TIt end, TValue value, TTrans trans)
{
   const auto saved_end = end;
   while (begin != end) {
      auto mid = begin + std::distance(begin, end) / 2;
      const auto comp = trans(*mid);
      if (comp < value) {
         begin = std::next(mid);
      } else if (comp > value) {
         end = mid;
      } else {
         return mid;
      }
   }
   return saved_end;
}

}// namespace triglav