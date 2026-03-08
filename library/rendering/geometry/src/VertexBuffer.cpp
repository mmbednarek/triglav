#include "VertexBuffer.hpp"

namespace triglav::geometry {

namespace {


[[maybe_unused]] [[nodiscard]] MemorySize get_component_offset(const VertexComponentFlags all_components,
                                                               const VertexComponent target_component)
{
   MemorySize result{};
   if (all_components & VertexComponent::Core) {
      if (target_component == VertexComponent::Core) {
         return result;
      }
      result += sizeof(VertexComponentCore);
   }
   if (all_components & VertexComponent::Texture) {
      if (target_component == VertexComponent::Texture) {
         return result;
      }
      result += sizeof(VertexComponentTexture);
   }
   if (all_components & VertexComponent::NormalMap) {
      if (target_component == VertexComponent::NormalMap) {
         return result;
      }
      result += sizeof(VertexComponentNormalMap);
   }
   if (all_components & VertexComponent::Skeleton) {
      if (target_component == VertexComponent::Skeleton) {
         return result;
      }
      result += sizeof(VertexComponentSkeleton);
   }

   assert(false);
   return result;
}

}// namespace

VertexView::VertexView(const VertexComponentFlags components, const std::span<u8> data) :
    m_components(components),
    m_data(data)
{
}

MemorySize VertexView::stride() const
{
   return get_vertex_size(m_components);
}

u8* VertexView::vertex_start(const MemorySize index)
{
   assert(this->stride() * (index + 1) <= m_data.size());
   return m_data.data() + this->stride() * index;
}

MemorySize VertexView::component_offset(const VertexComponent component) const
{
   return get_component_offset(m_components, component);
}

u8* VertexView::vertex_component(const VertexComponent component, const MemorySize index)
{
   return this->vertex_start(index) + this->component_offset(component);
}

u8* VertexView::data()
{
   return m_data.data();
}

MemorySize VertexView::size() const
{
   return m_data.size();
}

VertexBuffer::VertexBuffer() = default;

u32 VertexBuffer::allocate_group(const VertexComponentFlags components, const MaterialName material, const MemorySize vertex_count,
                                 const MemorySize index_offset, const MemorySize index_size)
{
   const auto vert_size = get_vertex_size(components);
   const auto group_size = vert_size * vertex_count;
   const auto offset = m_allocated_size;
   m_allocated_size += group_size;
   if (m_data.size() < m_allocated_size) {
      m_data.resize(m_allocated_size);
   }

   m_vertex_groups.emplace_back(components, material, offset, group_size, index_offset, index_size);
   return m_vertex_groups.size() - 1;
}

VertexView VertexBuffer::group(const u32 index)
{
   const auto& group = m_vertex_groups.at(index);
   return VertexView(group.components, {m_data.data() + group.vertex_offset, group.vertex_size});
}

const u8* VertexBuffer::data() const
{
   return m_data.data();
}

u8* VertexBuffer::data()
{
   return m_data.data();
}

MemorySize VertexBuffer::size() const
{
   return m_data.size();
}

const std::vector<VertexGroup>& VertexBuffer::vertex_groups() const
{
   return m_vertex_groups;
}

}// namespace triglav::geometry
