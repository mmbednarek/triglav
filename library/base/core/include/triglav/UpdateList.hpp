#pragma once

#include "triglav/Int.hpp"

#include <map>
#include <set>
#include <vector>

namespace triglav {

template<typename W, typename T>
concept UpdateWriter = requires(W& writer, T obj, u32 srcIndex, u32 dstIndex) {
   { writer.set_object(dstIndex, obj) };
   { writer.move_object(srcIndex, dstIndex) };
};

template<typename TKey, typename TValue>
class UpdateList
{
 public:
   void add_or_update(TKey key, TValue&& value);
   void remove(TKey key);
   [[nodiscard]] u32 top_index() const;
   [[nodiscard]] const std::map<TKey, u32>& key_map() const;

   void write_to_buffers(UpdateWriter<TValue> auto& writer);

 private:
   void register_mapping(TKey key, u32 index);
   [[nodiscard]] u32 remove_mapping_by_key(TKey key);
   [[nodiscard]] TKey remove_mapping_by_index(u32 index);

   std::map<TKey, u32> m_keyToIndex;
   std::map<u32, TKey> m_indexToKey;
   std::vector<std::pair<TKey, TValue>> m_additions;
   std::set<TKey> m_removals;
   u32 m_indexCount{};
};

}// namespace triglav

#include "UpdateList.inl"