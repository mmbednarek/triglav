#include "BufferHeap.hpp"

#include "Device.hpp"

namespace triglav::graphics_api {

constexpr auto CHUNK_SIZE = 1024ull;

using ChunkCount = Index;

namespace {

ChunkCount memory_size_to_chunk_count(const MemorySize size)
{
   return size % CHUNK_SIZE == 0 ? size / CHUNK_SIZE : size / CHUNK_SIZE + 1;
}

}// namespace

BufferHeap::BufferHeap(Device& device, const BufferUsageFlags usageFlags, const MemorySize chunkCountPerPage) :
    m_device(device),
    m_usageFlags(usageFlags),
    m_chunkCount(chunkCountPerPage)
{
}

BufferHeap::~BufferHeap()
{
   for (const auto& page : m_pages) {
      auto* node = page.headNode;
      while (node != nullptr) {
         auto* next = node->next;
         delete node;
         node = next;
      }
   }
}

BufferHeap::Section BufferHeap::allocate_section(const MemorySize requiredSize)
{
   const auto requiredChunks = memory_size_to_chunk_count(requiredSize);
   assert(requiredChunks <= m_chunkCount);

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

      pageIt = this->find_available_page(pageIt + 1, requiredChunks);
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

   return Section(buffer, offset, requiredChunks * CHUNK_SIZE, currNode);
}

void BufferHeap::release_section(const Section& section)
{
   section.node->isFree = true;
   while (section.node->next != nullptr && section.node->next->isFree) {
      section.node->chunkCount += section.node->next->chunkCount;
      section.node->next = section.node->next->next;
   }
}

BufferHeap::Page& BufferHeap::allocate_page()
{
   return m_pages.emplace_back(Page{
      .buffer = std::make_unique<Buffer>(GAPI_CHECK(m_device.create_buffer(m_usageFlags, m_chunkCount * CHUNK_SIZE))),
      .availableChunks = m_chunkCount,
      .headNode = new Node{.chunkCount = m_chunkCount, .isFree = true, .next = nullptr},
   });
}

std::vector<BufferHeap::Page>::iterator BufferHeap::find_available_page(const std::vector<Page>::iterator start, const Index chunkCount)
{
   for (auto it = start; it != m_pages.end(); ++it) {
      if (chunkCount <= it->availableChunks) {
         return it;
      }
   }

   return m_pages.end();
}

}// namespace triglav::graphics_api