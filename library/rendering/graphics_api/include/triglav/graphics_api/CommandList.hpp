#pragma once

#include "Buffer.hpp"
#include "DescriptorWriter.hpp"
#include "GraphicsApi.hpp"
#include "Pipeline.hpp"

#include <span>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(CommandPool, Device)

class Pipeline;
class QueryPool;
class DescriptorWriter;

namespace ray_tracing {
class ShaderBindingTable;
}

enum class SubmitType
{
   Normal,
   OneTime,
};

class CommandList
{
 public:
   CommandList(Device& device, VkCommandBuffer command_buffer, VkCommandPool command_pool, WorkTypeFlags work_types);
   ~CommandList();

   CommandList(const CommandList& other) = delete;
   CommandList& operator=(const CommandList& other) = delete;
   CommandList(CommandList&& other) noexcept;
   CommandList& operator=(CommandList&& other) noexcept;

   [[nodiscard]] Status reset() const;
   [[nodiscard]] Status begin(SubmitType type = SubmitType::Normal) const;
   [[nodiscard]] Status finish() const;

   [[nodiscard]] VkCommandBuffer vulkan_command_buffer() const;

   void bind_pipeline(const Pipeline& pipeline);
   void bind_descriptor_set(PipelineType pipeline_type, const DescriptorView& descriptor_set) const;
   void draw_primitives(int vertex_count, int vertex_offset);
   void draw_primitives(int vertex_count, int vertex_offset, int instance_count, int first_instance);
   void draw_indexed_primitives(int index_count, int index_offset, int vertex_offset, int instance_count = 1, int first_instance = 0);
   void dispatch(u32 x, u32 y, u32 z);
   void dispatch_indirect(const Buffer& indirect_buffer) const;
   void bind_vertex_buffer(const Buffer& buffer, uint32_t layout_index) const;
   void bind_index_buffer(const Buffer& buffer) const;
   void copy_buffer(const Buffer& source, const Buffer& dest) const;
   void copy_buffer(const Buffer& source, const Buffer& dest, u32 src_offset, u32 dst_offset, u32 size) const;
   void copy_buffer_to_texture(const Buffer& source, const Texture& destination, int mip_level = 0) const;
   void copy_texture_to_buffer(const Texture& source, const Buffer& destination, int mip_level = 0,
                               TextureState src_texture_state = TextureState::TransferSrc) const;
   void copy_texture(const Texture& source, TextureState src_state, const Texture& destination, TextureState dst_state, u32 src_mip = 0,
                     u32 dst_mip = 0) const;
   void copy_texture_region(const Texture& source, TextureState src_state, Vector2i src_offset, const Texture& destination,
                            TextureState dst_state, Vector2i dst_offset, Vector2u size, u32 src_mip = 0, u32 dst_mip = 0) const;
   void push_constant_ptr(PipelineStageFlags stages, const void* ptr, size_t size, size_t offset = 0) const;

   void texture_barrier(PipelineStageFlags source_stage, PipelineStageFlags target_stage, std::span<const TextureBarrierInfo> infos) const;
   void texture_barrier(PipelineStageFlags source_stage, PipelineStageFlags target_stage, const TextureBarrierInfo& info) const;
   void execution_barrier(PipelineStageFlags source_stage, PipelineStageFlags target_stage) const;
   void buffer_barrier(PipelineStageFlags source_stage, PipelineStageFlags target_stage, std::span<const BufferBarrier> barriers) const;

   void blit_texture(const Texture& source_tex, const TextureRegion& source_region, const Texture& target_tex,
                     const TextureRegion& target_region) const;
   void reset_timestamp_array(const QueryPool& timestamp_array, u32 first, u32 count) const;
   void write_timestamp(PipelineStage stage, const QueryPool& timestamp_array, u32 timestamp_index) const;
   void push_descriptors(u32 set_index, DescriptorWriter& writer, PipelineType pipeline_type) const;
   void draw_indexed_indirect_with_count(const Buffer& draw_call_buffer, const Buffer& count_buffer, u32 max_draw_calls, u32 stride,
                                         u32 count_buffer_offset);
   void draw_indirect_with_count(const Buffer& draw_call_buffer, const Buffer& count_buffer, u32 max_draw_calls, u32 stride);
   void update_buffer(const Buffer& buffer, u32 offset, u32 size, const void* data) const;

   void begin_rendering(const RenderingInfo& info) const;
   void end_rendering() const;

   void bind_raw_uniform_buffer(u32 binding, const Buffer& buffer);
   void bind_storage_buffer(u32 binding, const Buffer& buffer);
   void bind_storage_buffer(u32 binding, const Buffer& buffer, u32 offset, u32 size);
   void bind_acceleration_structure(u32 binding, const ray_tracing::AccelerationStructure& acc_structure);
   void set_viewport(Vector4 dimensions, float min_depth, float max_depth) const;

   void trace_rays(const ray_tracing::ShaderBindingTable& binding_table, glm::ivec3 extent);

   template<typename TValue>
   void bind_uniform_buffer(const uint32_t binding, const TValue& buffer)
   {
      this->bind_raw_uniform_buffer(binding, buffer.buffer());
   }

   void bind_texture_image(u32 binding, const Texture& texture);
   void bind_texture_view_image(u32 binding, const TextureView& texture);
   void bind_texture(u32 binding, const Texture& texture);
   void bind_texture_array(u32 binding, std::span<const Texture*> textures);
   void bind_storage_image(u32 binding, const Texture& texture);
   void bind_storage_image_view(u32 binding, const TextureView& texture);
   void begin_query(const QueryPool& query_pool, u32 query_index) const;
   void end_query(const QueryPool& query_pool, u32 query_index) const;

   template<typename TIndexArray>
   void bind_index_array(const TIndexArray& array) const
   {
      this->bind_index_buffer(array.buffer());
   }

   template<typename TVertexArray>
   void bind_vertex_array(const TVertexArray& array, const uint32_t binding = 0) const
   {
      this->bind_vertex_buffer(array.buffer(), binding);
   }

   template<typename TPushConstant>
   void push_constant(const PipelineStage stage, TPushConstant& push_constant) const
   {
      this->push_constant_ptr(stage, &push_constant, sizeof(TPushConstant));
   }

   template<typename TMesh>
   void draw_mesh(const TMesh& mesh)
   {
      this->bind_vertex_array(mesh.vertices, 0);
      this->bind_index_array(mesh.indices);
      this->draw_indexed_primitives(mesh.indices.count(), 0, 0);
   }

   [[nodiscard]] WorkTypeFlags work_types() const;
   [[nodiscard]] uint64_t triangle_count() const;

   [[nodiscard]] Device& device()
   {
      return m_device;
   }

   void set_debug_name(std::string_view name) const;

 private:
   void handle_pending_descriptors(PipelineType pipeline_type);

   Device& m_device;

   VkCommandBuffer m_command_buffer;
   VkCommandPool m_command_pool;
   VkPipelineLayout m_bound_pipeline_layout{};
   WorkTypeFlags m_work_types;
   mutable uint64_t m_triangle_count{};
   DescriptorWriter m_descriptor_writer;
   bool m_has_pending_descriptors{false};
};

}// namespace triglav::graphics_api
