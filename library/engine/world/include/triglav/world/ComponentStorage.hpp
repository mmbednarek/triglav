#pragma once

#include "World.hpp"

#include "triglav/Macros.hpp"
#include "triglav/io/Stream.hpp"
#include "triglav/memory/HeapAllocator.hpp"

#include <map>
#include <vector>

namespace triglav::world {

constexpr MemorySize COMPONENT_BUCKET_SIZE_LOG2 = 8;
constexpr MemorySize COMPONENT_BUCKET_SIZE = 1 << COMPONENT_BUCKET_SIZE_LOG2;

[[nodiscard]] std::pair<mem_size, mem_size> calc_component_stride_and_offset(mem_size comp_size, mem_size comp_align);

class ComponentStorage;

class ComponentIterator
{
   using iterator_category = std::forward_iterator_tag;
   using value_type = std::pair<EntityID, void*>;
   using difference_type = std::ptrdiff_t;
   using pointer = value_type*;
   using reference = value_type&;

 public:
   ComponentIterator(u32 index, const ComponentStorage* storage);

   ComponentIterator& operator++();
   [[nodiscard]] value_type operator*() const;
   [[nodiscard]] bool operator!=(const ComponentIterator& other) const;

 private:
   u32 m_index;
   const ComponentStorage* m_storage;
};

class ComponentStorage
{
   friend class ComponentIterator;

 public:
   explicit ComponentStorage(ComponentID component_id);
   ComponentStorage();
   ~ComponentStorage();

   TG_DELETE_COPY(ComponentStorage)
   TG_DEFAULT_MOVE(ComponentStorage)

   u32 allocate_component(EntityID entity_id);
   void allocate_components(std::span<EntityID> entity_ids);
   [[nodiscard]] void* get_component(u32 index) const;
   [[nodiscard]] std::pair<EntityID, void*> get_component_with_eid(u32 index) const;
   [[nodiscard]] void* get_component_by_entity_id(EntityID id) const;
   [[nodiscard]] mem_size count() const;
   [[nodiscard]] bool is_empty() const;
   [[nodiscard]] bool contains_entity(EntityID entity_id) const;
   bool serialize(io::IWriter& writer) const;
   bool deserialize(io::IReader& reader);

   ComponentIterator begin() const;
   ComponentIterator end() const;

 private:
   std::vector<u8*> m_buckets;
   std::map<EntityID, u32> m_sparse_mapping;
   ComponentID m_component_id;
   mem_size m_count = 0;
   mem_size m_component_stride = 0;
   mem_size m_entity_id_offset = 0;
};

}// namespace triglav::world
