#include "ray_tracing/AccelerationStructurePool.hpp"
#include "Buffer.hpp"
#include "Device.hpp"

#include "triglav/desktop/ISurfaceEventListener.hpp"

#include <spdlog/spdlog.h>
#include <ranges>

namespace triglav::graphics_api::ray_tracing {

constexpr auto CHUNK_SIZE = 1024ull;
constexpr auto CHUNK_COUNT = 2048ull;
constexpr auto PAGE_SIZE = CHUNK_SIZE * CHUNK_COUNT;

using ChunkCount = Index;

namespace {

ChunkCount memory_size_to_chunk_count(const MemorySize size)
{
   return size % CHUNK_SIZE == 0 ? size / CHUNK_SIZE : size / CHUNK_SIZE + 1;
}

}// namespace

AccelerationStructurePool::AccelerationStructurePool(Device& device) :
    m_device(device)
{
}

AccelerationStructurePool::~AccelerationStructurePool()
{
   for (const auto& as : m_asToNodeMap | std::views::keys) {
      delete as;
   }

   for (const auto& buff : m_scratchBuffers) {
      delete buff;
   }

   for (const auto& page : m_pages) {
      auto* node = page.headNode;
      while (node != nullptr) {
         auto* next = node->next;
         delete node;
         node = next;
      }
   }
}

AccelerationStructure* AccelerationStructurePool::acquire_acceleration_structure(AccelerationStructureType type, MemorySize size)
{
   spdlog::info("as-pool: Acquiring {} acceleration structure, requested size {}",
                type == AccelerationStructureType::TopLevel ? "top level" : "bottom level", size);

   const auto requiredChunks = memory_size_to_chunk_count(size);
   assert(requiredChunks <= CHUNK_COUNT);

   auto pageIt = this->find_available_page(m_pages.begin(), requiredChunks);

   MemorySize offset{0};

   Node* currNode{};
   while (pageIt != m_pages.end()) {
      currNode = pageIt->headNode;
      while (currNode != nullptr) {
         if (currNode->isFree && requiredChunks < currNode->chunkCount) {
            break;
         }
         offset += currNode->chunkCount * CHUNK_SIZE;
         currNode = currNode->next;
      }
      if (currNode != nullptr) {
         break;
      }

      pageIt = this->find_available_page(pageIt+1, requiredChunks);
   }

   Buffer* buffer{};
   if (pageIt == m_pages.end()) {
      auto& page = this->allocate_page();
      currNode = page.headNode;
      page.availableChunks -= requiredChunks;
      buffer = page.buffer.get();
   } else {
      pageIt->availableChunks -= requiredChunks;
      buffer = pageIt->buffer.get();
   }

   if (currNode->chunkCount == requiredChunks) {
      currNode->isFree = false;
   } else {
      auto* newNode = new Node;
      newNode->chunkCount = currNode->chunkCount - requiredChunks;
      newNode->isFree = true;
      newNode->next = currNode->next;

      currNode->isFree = false;
      currNode->chunkCount = requiredChunks;
      currNode->next = newNode;
   }

   auto as = GAPI_CHECK(m_device.create_acceleration_structure(type, *buffer, offset, requiredChunks * CHUNK_SIZE));
   auto* result = new AccelerationStructure(std::move(as));
   m_asToNodeMap.emplace(result, currNode);
   return result;
}

void AccelerationStructurePool::release_acceleration_structure(AccelerationStructure* as)
{
   spdlog::info("as-pool: Releasing acceleration structure");

   auto* node = m_asToNodeMap.at(as);
   m_asToNodeMap.erase(as);

   node->isFree = true;
   while (node->next != nullptr && node->next->isFree) {
      node->chunkCount += node->next->chunkCount;
      node->next = node->next->next;
   }

   delete as;
}

Buffer* AccelerationStructurePool::allocate_scratch_buffer(MemorySize size)
{
   // TODO: Reuse allocated buffers.
   spdlog::info("as-pool: Acquiring scratch buffer, requested size {}", size);

   auto buffer = GAPI_CHECK(m_device.create_buffer(BufferUsage::AccelerationStructure | BufferUsage::StorageBuffer, size));
   auto* result = new Buffer(std::move(buffer));
   m_scratchBuffers.emplace(result);
   return result;
}

void AccelerationStructurePool::release_scratch_buffer(Buffer* buff)
{
   spdlog::info("as-pool: Releasing scratch buffer");
   m_scratchBuffers.erase(buff);
   delete buff;
}

AccelerationStructurePool::Page& AccelerationStructurePool::allocate_page()
{
   return m_pages.emplace_back(Page{
      .buffer = std::make_unique<Buffer>(GAPI_CHECK(m_device.create_buffer(BufferUsage::AccelerationStructure, PAGE_SIZE))),
      .availableChunks = CHUNK_COUNT,
      .headNode = new Node{.chunkCount = CHUNK_COUNT, .isFree = true, .next = nullptr},
   });
}

std::vector<AccelerationStructurePool::Page>::iterator
AccelerationStructurePool::find_available_page(const std::vector<Page>::iterator start, const Index chunkCount)
{
   for (auto it = start; it != m_pages.end(); ++it) {
      if (chunkCount <= it->availableChunks) {
         return it;
      }
   }

   return m_pages.end();
}

}// namespace triglav::graphics_api::ray_tracing
