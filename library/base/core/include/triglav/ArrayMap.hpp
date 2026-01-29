#pragma once

#include "Int.hpp"
#include "Template.hpp"

#include <algorithm>
#include <vector>

namespace triglav {

template<typename TKey, typename TValue>
class ArrayMap
{
 public:
   using Container = std::vector<std::pair<TKey, TValue>>;
   using iterator = typename Container::iterator;
   using const_iterator = typename Container::const_iterator;

   ArrayMap() = default;

   ArrayMap(std::initializer_list<std::pair<TKey, TValue>> values) :
       m_values(std::move(values))
   {
      this->sort_keys();
   }

   [[nodiscard]] MemorySize size() const
   {
      return m_values.size();
   }

   [[nodiscard]] TValue& operator[](const TKey& key)
   {
      const auto it = this->find(key);
      if (it == m_values.end()) {
         return this->append(key);
      }
      return it->second;
   }

   [[nodiscard]] TValue& at(const TKey& key)
   {
      return this->find(key)->second;
   }

   [[nodiscard]] const TValue& at(const TKey& key) const
   {
      return this->find(key)->second;
   }

   [[nodiscard]] iterator find(const TKey& key)
   {
      return find_binary(m_values.begin(), m_values.end(), key, [](const auto& pair) { return pair.first; });
   }

   TValue& append(const TKey& key)
   {
      m_values.emplace_back(key, TValue{});
      this->sort_keys();
      return this->find(key)->second;
   }

   [[nodiscard]] iterator begin()
   {
      return m_values.begin();
   }

   [[nodiscard]] iterator end()
   {
      return m_values.end();
   }

   [[nodiscard]] const_iterator begin() const
   {
      return m_values.begin();
   }

   [[nodiscard]] const_iterator end() const
   {
      return m_values.end();
   }

 private:
   void sort_keys()
   {
      std::sort(m_values.begin(), m_values.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
   }

   Container m_values;
};

}// namespace triglav