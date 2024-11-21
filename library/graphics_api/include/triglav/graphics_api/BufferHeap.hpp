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
      Index chunkCount;
      bool isFree;
      Node* next;
   };

   struct Page
   {
      std::unique_ptr<Buffer> buffer;
      Index availableChunks;
      Node* headNode;
   };

   struct Section
   {
      Buffer* buffer;
      MemorySize offset;
      MemorySize size;
      Node* node;
   };

   BufferHeap(Device& device, BufferUsageFlags usageFlags, MemorySize chunkCountPerPage = 8192);
   ~BufferHeap();

   BufferHeap(const BufferHeap& other) = delete;
   BufferHeap(BufferHeap&& other) noexcept = delete;
   BufferHeap& operator=(const BufferHeap& other) = delete;
   BufferHeap& operator=(BufferHeap&& other) noexcept = delete;

   Section allocate_section(MemorySize requiredSize);
   void release_section(const Section& section);

 private:
   Page& allocate_page();
   std::vector<Page>::iterator find_available_page(std::vector<Page>::iterator start, Index chunkCount);

   // References
   Device& m_device;

   // Containers
   std::vector<Page> m_pages;

   // State
   BufferUsageFlags m_usageFlags;
   MemorySize m_chunkCount;
};

}// namespace triglav::graphics_api