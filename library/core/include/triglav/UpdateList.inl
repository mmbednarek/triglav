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
void UpdateList<TKey, TValue>::write_to_buffers(UpdateWriter<TValue> auto& writer)
{
   // For additions with existing keys, we would like maintain the same key
   for (const auto& [key, value] : m_additions) {
      auto it = m_keyToIndex.find(key);
      if (it != m_keyToIndex.end()) {
         writer.set_object(it->second, value);
      }
   }
   std::erase_if(m_additions, [this](const auto& pair) { return m_keyToIndex.contains(pair.first); });

   std::set<u32> removal_indices{};
   for (const auto rem_key : m_removals) {
      removal_indices.emplace(m_keyToIndex.at(rem_key));
   }

   const auto target_count = m_indexCount - m_removals.size() + m_additions.size();

   // First we use additions to remove the objects
   auto addition_it = m_additions.begin();
   auto removal_it = m_removals.begin();
   while (addition_it != m_additions.end() && removal_it != m_removals.end()) {
      const auto rem_index = this->remove_mapping_by_key(*removal_it);
      if (rem_index >= target_count) {
         // ignore removed by shrinking
         ++removal_it;
         continue;
      }

      this->register_mapping(addition_it->first, rem_index);
      writer.set_object(rem_index, addition_it->second);

      ++addition_it;
      ++removal_it;
   }

   // If there are remaining additions append them to the end
   auto dst_index = m_indexCount;
   while (addition_it != m_additions.end()) {
      this->register_mapping(addition_it->first, dst_index);
      writer.set_object(dst_index++, addition_it->second);
      ++addition_it;
   }

   // If there are existing removals we need to move non-removed objects from the top
   // to index of the removal
   auto src_index = target_count;
   while (removal_it != m_removals.end()) {
      auto rem_index = this->remove_mapping_by_key(*removal_it);
      if (rem_index >= target_count) {
         // ignore removed by shrinking
         ++removal_it;
         continue;
      }

      // ignore the source indices that are assigned for removals
      while (removal_indices.contains(src_index)) {
         ++src_index;
      }

      // assigned a new mapping and move the object
      auto index_key = this->remove_mapping_by_index(src_index);
      this->register_mapping(index_key, rem_index);
      writer.move_object(src_index, rem_index);
      ++src_index;
      ++removal_it;
   }

   assert(m_keyToIndex.size() == target_count);
   assert(m_indexToKey.size() == target_count);

   m_indexCount = target_count;
   m_removals.clear();
   m_additions.clear();
}

template<typename TKey, typename TValue>
[[nodiscard]] const std::map<TKey, u32>& UpdateList<TKey, TValue>::key_map() const
{
   return m_keyToIndex;
}

template<typename TKey, typename TValue>
void UpdateList<TKey, TValue>::register_mapping(const TKey key, const u32 index)
{
   assert(!m_indexToKey.contains(index));
   m_indexToKey[index] = key;
   m_keyToIndex[key] = index;
}

template<typename TKey, typename TValue>
u32 UpdateList<TKey, TValue>::remove_mapping_by_key(const TKey key)
{
   const auto index = m_keyToIndex.at(key);
   m_keyToIndex.erase(key);
   m_indexToKey.erase(index);
   return index;
}

template<typename TKey, typename TValue>
TKey UpdateList<TKey, TValue>::remove_mapping_by_index(const u32 index)
{
   const auto key = m_indexToKey.at(index);
   m_keyToIndex.erase(key);
   m_indexToKey.erase(index);
   return key;
}

}// namespace triglav
