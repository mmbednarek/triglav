#pragma once

#include "Buffer.hpp"
#include "DescriptorWriter.hpp"
#include "GraphicsApi.hpp"
#include "Pipeline.hpp"

#include <glm/vec3.hpp>
#include <span>

namespace triglav::graphics_api {

DECLARE_VLK_WRAPPED_CHILD_OBJECT(CommandPool, Device)

class Pipeline;
class Framebuffer;
class RenderTarget;
class TimestampArray;
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
   CommandList(Device& device, VkCommandBuffer commandBuffer, VkCommandPool commandPool, WorkTypeFlags workTypes);
   ~CommandList();

   CommandList(const CommandList& other) = delete;
   CommandList& operator=(const CommandList& other) = delete;
   CommandList(CommandList&& other) noexcept;
   CommandList& operator=(CommandList&& other) noexcept;

   [[nodiscard]] Status reset() const;
   [[nodiscard]] Status begin(SubmitType type = SubmitType::Normal) const;
   [[nodiscard]] Status finish() const;

   [[nodiscard]] VkCommandBuffer vulkan_command_buffer() const;

   void begin_render_pass(const Framebuffer& framebuffer, std::span<ClearValue> clearValues) const;
   void end_render_pass() const;
   void bind_pipeline(const Pipeline& pipeline);
   void bind_descriptor_set(const DescriptorView& descriptorSet) const;
   void draw_primitives(int vertexCount, int vertexOffset);
   void draw_primitives(int vertexCount, int vertexOffset, int instanceCount, int firstInstance);
   void draw_indexed_primitives(int indexCount, int indexOffset, int vertexOffset);
   void dispatch(u32 x, u32 y, u32 z);
   void bind_vertex_buffer(const Buffer& buffer, uint32_t layoutIndex) const;
   void bind_index_buffer(const Buffer& buffer) const;
   void copy_buffer(const Buffer& source, const Buffer& dest) const;
   void copy_buffer(const Buffer& source, const Buffer& dest, u32 srcOffset, u32 dstOffset, u32 size) const;
   void copy_buffer_to_texture(const Buffer& source, const Texture& destination, int mipLevel = 0) const;
   void copy_texture_to_buffer(const Texture& source, const Buffer& destination, int mipLevel = 0,
                               TextureState srcTextureState = TextureState::TransferSrc) const;
   void copy_texture(const Texture& source, TextureState srcState, const Texture& destination, TextureState dstState, u32 srcMip = 0,
                     u32 dstMip = 0) const;
   void push_constant_ptr(PipelineStage stage, const void* ptr, size_t size, size_t offset = 0) const;

   void texture_barrier(PipelineStageFlags sourceStage, PipelineStageFlags targetStage, std::span<const TextureBarrierInfo> infos) const;
   void texture_barrier(PipelineStageFlags sourceStage, PipelineStageFlags targetStage, const TextureBarrierInfo& info) const;
   void execution_barrier(PipelineStageFlags sourceStage, PipelineStageFlags targetStage) const;

   void blit_texture(const Texture& sourceTex, const TextureRegion& sourceRegion, const Texture& targetTex,
                     const TextureRegion& targetRegion) const;
   void reset_timestamp_array(const TimestampArray& timestampArray, u32 first, u32 count) const;
   void write_timestamp(PipelineStage stage, const TimestampArray& timestampArray, u32 timestampIndex) const;
   void push_descriptors(u32 setIndex, DescriptorWriter& writer, PipelineType pipelineType) const;
   void draw_indirect_with_count(const Buffer& drawCallBuffer, const Buffer& countBuffer, u32 maxDrawCalls, u32 stride);
   void update_buffer(const Buffer& buffer, u32 offset, u32 size, const void* data) const;

   void bind_raw_uniform_buffer(u32 binding, const Buffer& buffer);
   void bind_storage_buffer(u32 binding, const Buffer& buffer);
   void bind_storage_buffer(u32 binding, const Buffer& buffer, u32 offset, u32 size);
   void bind_acceleration_structure(u32 binding, const ray_tracing::AccelerationStructure& accStructure);

   void trace_rays(const ray_tracing::ShaderBindingTable& binding_table, glm::ivec3 extent);

   template<typename TValue>
   void bind_uniform_buffer(const uint32_t binding, const TValue& buffer)
   {
      this->bind_raw_uniform_buffer(binding, buffer.buffer());
   }

   void bind_texture_image(u32 binding, const Texture& texture);
   void bind_texture_view_image(u32 binding, const TextureView& texture);
   void bind_texture(u32 binding, const Texture& texture);
   void bind_texture_array(u32 binding, std::span<Texture*> textures);
   void bind_storage_image(u32 binding, const Texture& texture);
   void bind_storage_image_view(u32 binding, const TextureView& texture);

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
   void push_constant(const PipelineStage stage, TPushConstant& pushConstant) const
   {
      this->push_constant_ptr(stage, &pushConstant, sizeof(TPushConstant));
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

 private:
   void handle_pending_descriptors(PipelineType pipelineType);

   Device& m_device;

   VkCommandBuffer m_commandBuffer;
   VkCommandPool m_commandPool;
   VkPipelineLayout m_boundPipelineLayout{};
   WorkTypeFlags m_workTypes;
   mutable uint64_t m_triangleCount{};
   DescriptorWriter m_descriptorWriter;
   bool m_hasPendingDescriptors{false};
};

}// namespace triglav::graphics_api
