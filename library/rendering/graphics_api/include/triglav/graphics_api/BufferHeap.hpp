#pragma once

#include "GraphicsApi.hpp"
#include "triglav/Int.hpp"

#include <memory>
#include <vector>

namespace triglav::graphics_api {

class Buffer;
class Device;

class BufferHeap
{
 public:
   struct Node
   {
      Index chunk_count;
      bool is_free;
      Node* next;
   };

   struct Page
   {
      std::unique_ptr<Buffer> buffer;
      Index available_chunks;
      Node* head_node;
   };

   struct Section
   {
      Buffer* buffer;
      MemorySize offset;
      MemorySize size;
      Node* node;
   };

   BufferHeap(Device& device, BufferUsageFlags usage_flags, MemorySize chunk_count_per_page = 8192);
   ~BufferHeap();

   BufferHeap(const BufferHeap& other) = delete;
   BufferHeap(BufferHeap&& other) noexcept = delete;
   BufferHeap& operator=(const BufferHeap& other) = delete;
   BufferHeap& operator=(BufferHeap&& other) noexcept = delete;

   Section allocate_section(MemorySize required_size);
   void release_section(const Section& section);

 private:
   Page& allocate_page();
   std::vector<Page>::iterator find_available_page(std::vector<Page>::iterator start, Index chunk_count);

   // References
   Device& m_device;

   // Containers
   std::vector<Page> m_pages;

   // State
   BufferUsageFlags m_usage_flags;
   MemorySize m_chunk_count;
};

}// namespace triglav::graphics_api