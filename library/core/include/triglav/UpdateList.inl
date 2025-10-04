#pragma once

#include <cassert>

namespace triglav {

template<typename TKey, typename TValue>
void UpdateList<TKey, TValue>::add_or_update(TKey key, TValue&& value)
{
   auto it = std::ranges::find_if(m_additions, [key](const auto& pair) { return pair.first == key; });
   if (it != m_additions.end()) {
      it->second = std::forward<TValue>(value);
      return;
   }

   m_additions.emplace_back(key, std::forward<TValue>(value));
}

template<typename TKey, typename TValue>
void UpdateList<TKey, TValue>::remove(TKey key)
{
   m_removals.emplace(key);
}

template<typename TKey, typename TValue>
[[nodiscard]] u32 UpdateList<TKey, TValue>::top_index() const
{
   return m_indexCount;
}

template<typename TKey, typename TValue>
void UpdateList<TKey, TValue>::write_to_buffers(InsertRemoveWriter<TValue> auto& writer)
{
   for (const auto& [key, value] : m_additions) {
      auto it = m_keyMapping.find(key);
      if (it != m_keyMapping.end()) {
         writer.add_insertion(it->second, value);
      }
   }
   std::erase_if(m_additions, [this](const auto& pair) { return m_keyMapping.contains(pair.first); });

   std::set<u32> removal_indices;
   const auto targetCount = m_indexCount - m_removals.size() + m_additions.size();
   size_t validRemovals = 0;
   for (const auto rem : m_removals) {
      auto rem_index = m_keyMapping.at(rem);
      removal_indices.insert(rem_index);
      if (rem_index < targetCount) {
         ++validRemovals;
      }
   }

   if (m_additions.size() >= validRemovals) {
      auto it = m_additions.begin();
      for (auto& removal : m_removals) {
         const auto rem_index = m_keyMapping.at(removal);
         if (rem_index >= targetCount) {
            m_reservedIndices.erase(rem_index);
            m_keyMapping.erase(removal);
            continue;
         }

         m_keyMapping.erase(removal);

         assert(m_reservedIndices[rem_index] == removal);
         m_reservedIndices[rem_index] = it->first;
         m_keyMapping[it->first] = rem_index;
         writer.add_insertion(rem_index, it->second);
         ++it;
      }

      auto dstIndex = targetCount - (m_additions.end() - it);

      while (it != m_additions.end()) {
         assert(!m_reservedIndices.contains(dstIndex));
         m_reservedIndices[dstIndex] = it->first;
         m_keyMapping[it->first] = dstIndex;
         writer.add_insertion(dstIndex++, it->second);
         ++it;
      }
   } else {
      auto it = m_removals.begin();
      for (auto& [key, addition] : m_additions) {
         auto rem_index = m_keyMapping.at(*it);
         while (rem_index >= targetCount) {
            ++it;
            rem_index = m_keyMapping.at(*it);
         }

         m_keyMapping.erase(*it);

         assert(m_reservedIndices[rem_index] == *it);
         m_reservedIndices[rem_index] = key;

         m_keyMapping[key] = rem_index;
         writer.add_insertion(rem_index, addition);
         ++it;
      }

      auto index = targetCount;

      while (it != m_removals.end()) {
         auto rem_index = m_keyMapping.at(*it);
         if (rem_index >= targetCount) {
            m_reservedIndices.erase(rem_index);
            m_keyMapping.erase(*it);
            ++it;
            continue;
         }

         while (removal_indices.contains(index)) {
            ++index;
         }

         m_keyMapping.erase(*it);
         auto reverseKey = m_reservedIndices.at(index);
         m_keyMapping[reverseKey] = rem_index;
         m_reservedIndices.erase(index);
         m_reservedIndices[rem_index] = reverseKey;
         writer.add_removal(index, rem_index);
         ++index;
         ++it;
      }
   }
   m_indexCount = targetCount;

   m_removals.clear();
   m_additions.clear();
}

template<typename TKey, typename TValue>
[[nodiscard]] const std::map<TKey, u32>& UpdateList<TKey, TValue>::key_map() const
{
   return m_keyMapping;
}

template<typename TKey, typename TValue>
void UpdateList<TKey, TValue>::clear()
{
   m_indexCount = 0;
   m_keyMapping.clear();
   m_removals.clear();
   m_additions.clear();
}

}// namespace triglav
