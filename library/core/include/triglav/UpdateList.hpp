#pragma once

#include "triglav/Int.hpp"

#include <map>
#include <set>
#include <vector>

namespace triglav {

template<typename W, typename T>
concept InsertRemoveWriter = requires(W& writer, T obj, u32 index) {
   { writer.add_insertion(index, obj) };
   { writer.add_removal(index, index) };
};

template<typename TKey, typename TValue>
class UpdateList
{
 public:
   void add_or_update(TKey key, TValue&& value);
   void remove(TKey key);
   [[nodiscard]] u32 top_index() const;
   [[nodiscard]] const std::map<TKey, u32>& key_map() const;
   void clear();

   void write_to_buffers(InsertRemoveWriter<TValue> auto& writer);

 private:
   std::map<TKey, u32> m_keyMapping;
   std::vector<std::pair<TKey, TValue>> m_additions;
   std::set<TKey> m_removals;
   std::map<u32, TKey> m_reservedIndices;
   u32 m_indexCount{};
};

}// namespace triglav

#include "UpdateList.inl"