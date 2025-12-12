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

BufferHeap::BufferHeap(Device& device, const BufferUsageFlags usage_flags, const MemorySize chunk_count_per_page) :
    m_device(device),
    m_usage_flags(usage_flags),
    m_chunk_count(chunk_count_per_page)
{
}

BufferHeap::~BufferHeap()
{
   for (const auto& page : m_pages) {
      auto* node = page.head_node;
      while (node != nullptr) {
         auto* next = node->next;
         delete node;
         node = next;
      }
   }
}

BufferHeap::Section BufferHeap::allocate_section(const MemorySize required_size)
{
   const auto required_chunks = memory_size_to_chunk_count(required_size);
   assert(required_chunks <= m_chunk_count);

   auto page_it = this->find_available_page(m_pages.begin(), required_chunks);

   MemorySize offset{0};

   Node* curr_node{};
   while (page_it != m_pages.end()) {
      curr_node = page_it->head_node;
      while (curr_node != nullptr) {
         if (curr_node->is_free && required_chunks < curr_node->chunk_count) {
            break;
         }
         offset += curr_node->chunk_count * CHUNK_SIZE;
         curr_node = curr_node->next;
      }
      if (curr_node != nullptr) {
         break;
      }

      page_it = this->find_available_page(page_it + 1, required_chunks);
   }

   Buffer* buffer{};
   if (page_it == m_pages.end()) {
      auto& page = this->allocate_page();
      curr_node = page.head_node;
      page.available_chunks -= required_chunks;
      buffer = page.buffer.get();
   } else {
      page_it->available_chunks -= required_chunks;
      buffer = page_it->buffer.get();
   }

   if (curr_node->chunk_count == required_chunks) {
      curr_node->is_free = false;
   } else {
      auto* new_node = new Node;
      new_node->chunk_count = curr_node->chunk_count - required_chunks;
      new_node->is_free = true;
      new_node->next = curr_node->next;

      curr_node->is_free = false;
      curr_node->chunk_count = required_chunks;
      curr_node->next = new_node;
   }

   return Section(buffer, offset, required_chunks * CHUNK_SIZE, curr_node);
}

void BufferHeap::release_section(const Section& section)
{
   section.node->is_free = true;
   while (section.node->next != nullptr && section.node->next->is_free) {
      section.node->chunk_count += section.node->next->chunk_count;
      section.node->next = section.node->next->next;
   }
}

BufferHeap::Page& BufferHeap::allocate_page()
{
   return m_pages.emplace_back(Page{
      .buffer = std::make_unique<Buffer>(GAPI_CHECK(m_device.create_buffer(m_usage_flags, m_chunk_count * CHUNK_SIZE))),
      .available_chunks = m_chunk_count,
      .head_node = new Node{.chunk_count = m_chunk_count, .is_free = true, .next = nullptr},
   });
}

std::vector<BufferHeap::Page>::iterator BufferHeap::find_available_page(const std::vector<Page>::iterator start, const Index chunk_count)
{
   for (auto it = start; it != m_pages.end(); ++it) {
      if (chunk_count <= it->available_chunks) {
         return it;
      }
   }

   return m_pages.end();
}

}// namespace triglav::graphics_api