#include "ComponentStorage.hpp"

#include "ComponentManager.hpp"
#include "triglav/io/Serializer.hpp"
#include "triglav/meta/Binary.hpp"
#include "triglav/meta/Meta.hpp"

#include <cassert>

namespace triglav::world {

[[nodiscard]] std::pair<mem_size, mem_size> calc_component_stride_and_offset(const mem_size comp_size, const mem_size comp_align)
{
   assert(comp_align != 0 && comp_size != 0);

   mem_size stride = (comp_size + comp_align - 1) & ~(comp_align - 1);

   const mem_size diff = stride - comp_size;
   if (diff < sizeof(u32)) {
      // we cannot fit entity ID in the stride we need to extend it.
      // align the stride to u32
      stride = (stride + sizeof(u32) - 1) & ~(sizeof(u32) - 1);

      const auto offset = stride;
      stride += std::max(sizeof(u32), comp_align);
      return {stride, offset};
   }

   // we can fit entity ID
   return {stride, stride - sizeof(u32)};
}

ComponentIterator::ComponentIterator(const u32 index, const ComponentStorage* storage) :
    m_index(index),
    m_storage(storage)
{
}

ComponentIterator& ComponentIterator::operator++()
{
   ++m_index;
   return *this;
}

ComponentIterator::value_type ComponentIterator::operator*() const
{
   return m_storage->get_component_with_eid(m_index);
}

bool ComponentIterator::operator!=(const ComponentIterator& other) const
{
   return m_storage != other.m_storage || m_index != other.m_index;
}

ComponentStorage::ComponentStorage(const ComponentID component_id) :
    m_component_id(component_id)
{
   const auto& info = ComponentManager::the().info_by_id(component_id);
   std::tie(m_component_stride, m_entity_id_offset) = calc_component_stride_and_offset(info.data_size, info.data_alignment);
}

ComponentStorage::ComponentStorage() :
    m_component_id(~0u)
{
}

ComponentStorage::~ComponentStorage()
{
   for (const auto& bucket : m_buckets) {
      delete bucket;
   }
   m_buckets.clear();
}

u32 ComponentStorage::allocate_component(const EntityID entity_id)
{
   const mem_size index = m_count++;

   const mem_size bucket_id = index >> COMPONENT_BUCKET_SIZE_LOG2;
   if (bucket_id >= m_buckets.size()) {
      m_buckets.push_back(new u8[COMPONENT_BUCKET_SIZE * m_component_stride]);
   }

   void* ptr = this->get_component(index);
   std::memset(ptr, 0, m_component_stride);
   *reinterpret_cast<EntityID*>(static_cast<u8*>(ptr) + m_entity_id_offset) = entity_id;
   m_sparse_mapping[entity_id] = index;

   return index;
}

void ComponentStorage::allocate_components(const std::span<EntityID> entity_ids)
{
   const mem_size min_index = m_count;
   m_count += entity_ids.size();
   const mem_size max_index = m_count - 1;

   const mem_size max_bucket_id = max_index >> COMPONENT_BUCKET_SIZE_LOG2;

   while (max_bucket_id >= m_buckets.size()) {
      m_buckets.push_back(new u8[COMPONENT_BUCKET_SIZE * m_component_stride]);
   }

   auto entity_it = entity_ids.begin();
   for (mem_size index = min_index; index <= max_index; ++index) {
      const auto entity_id = *(entity_it++);
      void* ptr = this->get_component(index);
      *reinterpret_cast<EntityID*>(static_cast<u8*>(ptr) + m_entity_id_offset) = entity_id;
      m_sparse_mapping[entity_id] = index;
   }
}

void* ComponentStorage::get_component(const u32 index) const
{
   const mem_size bucket_id = index >> COMPONENT_BUCKET_SIZE_LOG2;
   const mem_size item_id = index & (COMPONENT_BUCKET_SIZE - 1);
   assert(bucket_id < m_buckets.size());

   return m_buckets.at(bucket_id) + m_component_stride * item_id;
}

std::pair<EntityID, void*> ComponentStorage::get_component_with_eid(const u32 index) const
{
   auto ptr = static_cast<u8*>(this->get_component(index));
   const EntityID id = *reinterpret_cast<EntityID*>(ptr + m_entity_id_offset);
   return {id, ptr};
}

void* ComponentStorage::get_component_by_entity_id(const EntityID id) const
{
   const auto it = m_sparse_mapping.find(id);
   if (it == m_sparse_mapping.end())
      return nullptr;
   return this->get_component(it->second);
}

mem_size ComponentStorage::count() const
{
   return m_count;
}

bool ComponentStorage::is_empty() const
{
   return m_count == 0;
}

bool ComponentStorage::contains_entity(const EntityID entity_id) const
{
   return m_sparse_mapping.contains(entity_id);
}

bool ComponentStorage::serialize(io::IWriter& writer) const
{
   const auto& info = ComponentManager::the().info_by_id(m_component_id);
   io::Serializer serializer(writer);

   // Writer component name
   if (!serializer.write_name(info.component_class).has_value())
      return false;

   // Write sparse set
   if (!serializer.write_u32(m_sparse_mapping.size()).has_value())
      return false;

   for (const auto& [key, value] : m_sparse_mapping) {
      if (!serializer.write_u32(key).has_value())
         return false;
      if (!serializer.write_u32(value).has_value())
         return false;
   }

   // Write components
   if (!serializer.write_u32(m_count).has_value())
      return false;

   for (u32 i = 0; i < m_count; ++i) {
      const meta::Ref comp{this->get_component(i), info.component_class};
      if (!meta::serialize_binary(writer, comp))
         return false;
   }

   return true;
}

bool ComponentStorage::deserialize(io::IReader& reader)
{
   io::Deserializer deserializer(reader);

   const Name component_name = deserializer.read_name();
   const auto cid = ComponentManager::the().component_id_by_class_name(component_name);
   if (!cid.has_value())
      return false;
   m_component_id = *cid;


   const auto& info = ComponentManager::the().info_by_id(m_component_id);
   std::tie(m_component_stride, m_entity_id_offset) = calc_component_stride_and_offset(info.data_size, info.data_alignment);

   const auto sparse_set_count = deserializer.read_u32();
   for (u32 i = 0; i < sparse_set_count; ++i) {
      const auto key = deserializer.read_u32();
      const auto value = deserializer.read_u32();
      m_sparse_mapping.emplace(key, value);
   }

   m_count = deserializer.read_u32();
   const mem_size max_bucket_id = m_count >> COMPONENT_BUCKET_SIZE_LOG2;
   while (max_bucket_id >= m_buckets.size()) {
      m_buckets.push_back(new u8[COMPONENT_BUCKET_SIZE * m_component_stride]);
   }

   for (u32 i = 0; i < m_count; ++i) {
      meta::Ref comp{this->get_component(i), info.component_class};
      meta::deserialize_binary(reader, comp);
   }

   return true;
}

ComponentIterator ComponentStorage::begin() const
{
   return {0, this};
}

ComponentIterator ComponentStorage::end() const
{
   return {static_cast<u32>(m_count), this};
}

}// namespace triglav::world
