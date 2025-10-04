#pragma once

#include <cassert>

namespace triglav {

template<typename TKey, typename TValue>
void UpdateList<TKey, TValue>::add(TKey key, TValue&& value)
{
   m_additions.emplace_back(key, std::forward<TValue>(value));
}

template<typename TKey, typename TValue>
void UpdateList<TKey, TValue>::update(const TKey key, TValue&& value)
{
   auto it = std::ranges::find_if(m_additions, [key](const auto& pair) { return pair.first == key; });
   if (it != m_additions.end()) {
      it->second = std::forward<TValue>(value);
      return;
   }

   auto itUp = std::ranges::find_if(m_updates, [key](const auto& pair) { return pair.first == key; });
   if (itUp != m_updates.end()) {
      itUp->second = std::forward<TValue>(value);
      return;
   }

  assert(m_keyMapping.contains(key));
  m_updates.emplace_back(key, std::forward<TValue>(value));
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
   for (const auto& update : m_updates) {
         const auto update_index = m_keyMapping.at(update.first);
         writer.add_insertion(update_index, update.second);
   }

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
         if (rem_index >= targetCount)
            continue;

         m_keyMapping.erase(removal);
         m_keyMapping.emplace(it->first, rem_index);
         writer.add_insertion(rem_index, it->second);
         ++it;
      }

      m_indexCount -= m_removals.size() - validRemovals;

      while (it != m_additions.end()) {
         m_keyMapping.emplace(it->first, m_indexCount);
         writer.add_insertion(m_indexCount++, it->second);
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
         m_keyMapping.emplace(key, rem_index);
         writer.add_insertion(rem_index, addition);
         ++it;
      }

      auto index = targetCount;

      while (it != m_removals.end()) {
         auto rem_index = m_keyMapping.at(*it);
         if (rem_index >= targetCount) {
            ++it;
            continue;
         }

         while (removal_indices.contains(index)) {
            ++index;
         }
         writer.add_removal(index, rem_index);
         ++index;
         ++it;
      }
   }
   m_indexCount = targetCount;

   m_updates.clear();
   m_removals.clear();
   m_additions.clear();
}

}// namespace triglav
