#pragma once

#include "Geometry.hpp"

#include <cassert>

namespace triglav::geometry {

template<typename T>
concept VertexComponentStruct = requires() {
   { T::Component } -> std::convertible_to<VertexComponent>;
};

class VertexView
{
 public:
   VertexView(VertexComponentFlags components, std::span<u8> data);

   [[nodiscard]] MemorySize stride() const;
   u8* vertex_start(MemorySize index);
   [[nodiscard]] MemorySize component_offset(VertexComponent component) const;
   u8* vertex_component(VertexComponent component, MemorySize index);

   template<VertexComponentStruct T>
   T& get(const MemorySize index)
   {
      assert(m_components & T::Component);
      return *reinterpret_cast<T*>(this->vertex_component(T::Component, index));
   }

   u8* data();
   [[nodiscard]] MemorySize size() const;

 private:
   VertexComponentFlags m_components;
   std::span<u8> m_data;
};

// Multilayout vertex buffer
class VertexBuffer
{
 public:
   VertexBuffer();

   u32 allocate_group(VertexComponentFlags components, MaterialName material, MemorySize vertex_count, MemorySize index_offset,
                      MemorySize index_size);
   [[nodiscard]] VertexView group(u32 index);

   [[nodiscard]] const u8* data() const;
   [[nodiscard]] u8* data();
   [[nodiscard]] MemorySize size() const;
   [[nodiscard]] const std::vector<VertexGroup>& vertex_groups() const;

 private:
   MemorySize m_allocated_size{};
   std::vector<u8> m_data;
   std::vector<VertexGroup> m_vertex_groups;
};

}// namespace triglav::geometry