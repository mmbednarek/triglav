#include "CommandList.hpp"

#include "DescriptorWriter.hpp"
#include "Device.hpp"
#include "Pipeline.hpp"
#include "QueryPool.hpp"
#include "Texture.hpp"
#include "ray_tracing/ShaderBindingTable.hpp"
#include "vulkan/DynamicProcedures.hpp"
#include "vulkan/Util.hpp"

#include "triglav/Ranges.hpp"

#include <cassert>

namespace triglav::graphics_api {

CommandList::CommandList(Device& device, const VkCommandBuffer command_buffer, const VkCommandPool command_pool,
                         const WorkTypeFlags work_types) :
    m_device(device),
    m_command_buffer(command_buffer),
    m_command_pool(command_pool),
    m_work_types(work_types),
    m_descriptor_writer(device)
{
}

CommandList::~CommandList()
{
   if (m_command_buffer != nullptr) {
      vkFreeCommandBuffers(m_device.vulkan_device(), m_command_pool, 1, &m_command_buffer);
   }
}

CommandList::CommandList(CommandList&& other) noexcept :
    m_device(other.m_device),
    m_command_buffer(std::exchange(other.m_command_buffer, nullptr)),
    m_command_pool(std::exchange(other.m_command_pool, nullptr)),
    m_work_types(std::exchange(other.m_work_types, WorkType::None)),
    m_descriptor_writer(std::move(other.m_descriptor_writer))
{
}

CommandList& CommandList::operator=(CommandList&& other) noexcept
{
   if (this == &other)
      return *this;
   m_command_buffer = std::exchange(other.m_command_buffer, nullptr);
   m_command_pool = std::exchange(other.m_command_pool, nullptr);
   m_work_types = std::exchange(other.m_work_types, WorkType::None);
   m_descriptor_writer = std::move(other.m_descriptor_writer);
   return *this;
}

Status CommandList::reset() const
{
   m_triangle_count = 0;
   if (vkResetCommandBuffer(m_command_buffer, 0) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
}

Status CommandList::begin(const SubmitType type) const
{
   m_triangle_count = 0;

   VkCommandBufferBeginInfo begin_info{};
   begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   if (type == SubmitType::OneTime) {
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   }

   if (vkBeginCommandBuffer(this->vulkan_command_buffer(), &begin_info) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status CommandList::finish() const
{
   if (vkEndCommandBuffer(m_command_buffer) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

VkCommandBuffer CommandList::vulkan_command_buffer() const
{
   return m_command_buffer;
}

void CommandList::bind_pipeline(const Pipeline& pipeline)
{
   vkCmdBindPipeline(m_command_buffer, vulkan::to_vulkan_pipeline_bind_point(pipeline.pipeline_type()), pipeline.vulkan_pipeline());
   m_bound_pipeline_layout = *pipeline.layout();
}

void CommandList::bind_descriptor_set(const PipelineType pipeline_type, const DescriptorView& descriptor_set) const
{
   const auto vulkan_descriptor_set = descriptor_set.vulkan_descriptor_set();
   vkCmdBindDescriptorSets(m_command_buffer, vulkan::to_vulkan_pipeline_bind_point(pipeline_type), m_bound_pipeline_layout, 0, 1,
                           &vulkan_descriptor_set, 0, nullptr);
}

void CommandList::draw_primitives(const int vertex_count, const int vertex_offset)
{
   this->draw_primitives(vertex_count, vertex_offset, 1, 0);
}

void CommandList::draw_primitives(const int vertex_count, const int vertex_offset, const int instance_count, const int first_instance)
{
   this->handle_pending_descriptors(PipelineType::Graphics);
   m_triangle_count += instance_count * (vertex_count / 3);
   vkCmdDraw(m_command_buffer, vertex_count, instance_count, vertex_offset, first_instance);
}

void CommandList::draw_indexed_primitives(const int index_count, const int index_offset, const int vertex_offset, const int instance_count,
                                          const int first_instance)
{
   this->handle_pending_descriptors(PipelineType::Graphics);
   m_triangle_count += index_count / 3;
   vkCmdDrawIndexed(m_command_buffer, index_count, instance_count, index_offset, vertex_offset, first_instance);
}

void CommandList::dispatch(const u32 x, const u32 y, const u32 z)
{
   // For some reason I need that XDDD
   assert(x != 0 && y != 0 && z != 0);

   this->handle_pending_descriptors(PipelineType::Compute);
   vkCmdDispatch(m_command_buffer, x, y, z);
}

void CommandList::dispatch_indirect(const Buffer& indirect_buffer) const
{
   vkCmdDispatchIndirect(m_command_buffer, indirect_buffer.vulkan_buffer(), 0);
}

void CommandList::bind_vertex_buffer(const Buffer& buffer, const u32 layout_index) const
{
   const std::array buffers{buffer.vulkan_buffer()};
   constexpr std::array<VkDeviceSize, 1> offsets{0};
   vkCmdBindVertexBuffers(m_command_buffer, layout_index, static_cast<u32>(buffers.size()), buffers.data(), offsets.data());
}

void CommandList::bind_index_buffer(const Buffer& buffer) const
{
   vkCmdBindIndexBuffer(m_command_buffer, buffer.vulkan_buffer(), 0, VK_INDEX_TYPE_UINT32);
}

void CommandList::copy_buffer(const Buffer& source, const Buffer& dest) const
{
   VkBufferCopy region{};
   region.size = source.size();
   vkCmdCopyBuffer(m_command_buffer, source.vulkan_buffer(), dest.vulkan_buffer(), 1, &region);
}

void CommandList::copy_buffer(const Buffer& source, const Buffer& dest, const u32 src_offset, const u32 dst_offset, const u32 size) const
{
   assert(size > 0);
   assert(src_offset < source.size());
   assert(src_offset + size <= source.size());
   assert(dst_offset < dest.size());
   assert(dst_offset + size <= dest.size());

   VkBufferCopy region{};
   region.srcOffset = src_offset;
   region.dstOffset = dst_offset;
   region.size = size;
   vkCmdCopyBuffer(m_command_buffer, source.vulkan_buffer(), dest.vulkan_buffer(), 1, &region);
}

void CommandList::copy_buffer_to_texture(const Buffer& source, const Texture& destination, const int mip_level) const
{
   VkBufferImageCopy region{};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;
   region.imageSubresource.aspectMask = vulkan::to_vulkan_aspect_flags(destination.usage_flags());
   region.imageSubresource.mipLevel = mip_level;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;
   region.imageOffset = {0, 0, 0};
   region.imageExtent = {destination.width(), destination.height(), 1};
   vkCmdCopyBufferToImage(m_command_buffer, source.vulkan_buffer(), destination.vulkan_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                          &region);
}

void CommandList::copy_texture_to_buffer(const Texture& source, const Buffer& destination, const int mip_level,
                                         const TextureState src_texture_state) const
{
   u32 width = source.width();
   u32 height = source.height();
   for (int level = 0; level < mip_level; ++level) {
      width /= 2;
      height /= 2;
      if (width == 0)
         width = 1;
      if (height == 0)
         height = 1;
   }

   VkBufferImageCopy region{};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;
   region.imageSubresource.aspectMask = vulkan::to_vulkan_aspect_flags(source.usage_flags());
   region.imageSubresource.mipLevel = mip_level;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;
   region.imageOffset = {0, 0, 0};
   region.imageExtent = {width, height, 1};
   vkCmdCopyImageToBuffer(m_command_buffer, source.vulkan_image(), vulkan::to_vulkan_image_layout(source.format(), src_texture_state),
                          destination.vulkan_buffer(), 1, &region);
}

void CommandList::copy_texture(const Texture& source, const TextureState src_state, const Texture& destination,
                               const TextureState dst_state, const u32 src_mip, const u32 dst_mip) const
{
   VkImageCopy image_copy{.srcSubresource{
                             .aspectMask = vulkan::to_vulkan_aspect_flags(source.usage_flags()),
                             .mipLevel = src_mip,
                             .baseArrayLayer = 0,
                             .layerCount = 1,
                          },
                          .srcOffset{0, 0, 0},
                          .dstSubresource{
                             .aspectMask = vulkan::to_vulkan_aspect_flags(destination.usage_flags()),
                             .mipLevel = dst_mip,
                             .baseArrayLayer = 0,
                             .layerCount = 1,
                          },
                          .dstOffset{0, 0, 0},
                          .extent{destination.width(), destination.height(), 1}};
   vkCmdCopyImage(m_command_buffer, source.vulkan_image(), vulkan::to_vulkan_image_layout(source.format(), src_state),
                  destination.vulkan_image(), vulkan::to_vulkan_image_layout(destination.format(), dst_state), 1, &image_copy);
}

void CommandList::copy_texture_region(const Texture& source, const TextureState src_state, const Vector2i src_offset,
                                      const Texture& destination, const TextureState dst_state, const Vector2i dst_offset,
                                      const Vector2u size, const u32 src_mip, const u32 dst_mip) const
{
   const VkImageCopy image_copy{.srcSubresource{
                                   .aspectMask = vulkan::to_vulkan_aspect_flags(source.usage_flags()),
                                   .mipLevel = src_mip,
                                   .baseArrayLayer = 0,
                                   .layerCount = 1,
                                },
                                .srcOffset{src_offset.x, src_offset.y, 0},
                                .dstSubresource{
                                   .aspectMask = vulkan::to_vulkan_aspect_flags(destination.usage_flags()),
                                   .mipLevel = dst_mip,
                                   .baseArrayLayer = 0,
                                   .layerCount = 1,
                                },
                                .dstOffset{dst_offset.x, dst_offset.y, 0},
                                .extent{size.x, size.y, 1}};
   vkCmdCopyImage(m_command_buffer, source.vulkan_image(), vulkan::to_vulkan_image_layout(source.format(), src_state),
                  destination.vulkan_image(), vulkan::to_vulkan_image_layout(destination.format(), dst_state), 1, &image_copy);
}

void CommandList::push_constant_ptr(const PipelineStageFlags stages, const void* ptr, const size_t size, const size_t offset) const
{
   assert(m_bound_pipeline_layout != nullptr);
   vkCmdPushConstants(m_command_buffer, m_bound_pipeline_layout, vulkan::to_vulkan_shader_stage_flags(stages), static_cast<u32>(offset),
                      static_cast<u32>(size), ptr);
}

void CommandList::texture_barrier(const PipelineStageFlags source_stage, const PipelineStageFlags target_stage,
                                  const std::span<const TextureBarrierInfo> infos) const
{
   std::vector<VkImageMemoryBarrier> barriers{};
   barriers.resize(infos.size());

   size_t index{};
   for (const auto& info : infos) {
      auto& barrier = barriers[index];
      ++index;

      assert(barrier.image == nullptr);

      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = vulkan::to_vulkan_image_layout(info.texture->format(), info.source_state);
      barrier.newLayout = vulkan::to_vulkan_image_layout(info.texture->format(), info.target_state);
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = info.texture->vulkan_image();
      barrier.subresourceRange.aspectMask = vulkan::to_vulkan_aspect_flags(info.texture->usage_flags());
      barrier.subresourceRange.baseMipLevel = info.base_mip_level;
      barrier.subresourceRange.levelCount = info.mip_level_count;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = 1;
      barrier.srcAccessMask = vulkan::to_vulkan_access_flags(source_stage, info.texture->format(), info.source_state);
      barrier.dstAccessMask = vulkan::to_vulkan_access_flags(target_stage, info.texture->format(), info.target_state);
   }

   const auto dst_stage_mask = vulkan::to_vulkan_pipeline_stage_flags(target_stage);
   assert(dst_stage_mask != 0);
   vkCmdPipelineBarrier(m_command_buffer, vulkan::to_vulkan_pipeline_stage_flags(source_stage), dst_stage_mask, 0, 0, nullptr, 0, nullptr,
                        static_cast<u32>(barriers.size()), barriers.data());
}

void CommandList::texture_barrier(const PipelineStageFlags source_stage, const PipelineStageFlags target_stage,
                                  const TextureBarrierInfo& info) const
{
   this->texture_barrier(source_stage, target_stage, std::span(&info, &info + 1));
}

void CommandList::execution_barrier(PipelineStageFlags source_stage, PipelineStageFlags target_stage) const
{
   vkCmdPipelineBarrier(m_command_buffer, vulkan::to_vulkan_pipeline_stage_flags(source_stage),
                        vulkan::to_vulkan_pipeline_stage_flags(target_stage), 0, 0, nullptr, 0, nullptr, 0, nullptr);
}

void CommandList::buffer_barrier(const PipelineStageFlags source_stage, const PipelineStageFlags target_stage,
                                 std::span<const BufferBarrier> barriers) const
{
   std::vector<VkBufferMemoryBarrier> vkBarriers{};
   vkBarriers.resize(barriers.size());

   for (const auto [i, barrier] : Enumerate(barriers)) {
      auto& vkBarrier = vkBarriers[i];
      vkBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      vkBarrier.srcAccessMask = vulkan::to_vulkan_access_flags(barrier.src_access);
      vkBarrier.dstAccessMask = vulkan::to_vulkan_access_flags(barrier.dst_access);
      vkBarrier.buffer = barrier.buffer->vulkan_buffer();
      vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkBarrier.offset = 0;
      vkBarrier.size = VK_WHOLE_SIZE;
   }

   vkCmdPipelineBarrier(m_command_buffer, vulkan::to_vulkan_pipeline_stage_flags(source_stage),
                        vulkan::to_vulkan_pipeline_stage_flags(target_stage), 0, 0, nullptr, static_cast<u32>(vkBarriers.size()),
                        vkBarriers.data(), 0, nullptr);
}

void CommandList::blit_texture(const Texture& source_tex, const TextureRegion& source_region, const Texture& target_tex,
                               const TextureRegion& target_region) const
{
   VkImageBlit blit{};
   blit.srcOffsets[0] = {static_cast<int>(source_region.offset_min.x), static_cast<int>(source_region.offset_min.y), 0};
   blit.srcOffsets[1] = {static_cast<int>(source_region.offset_max.x), static_cast<int>(source_region.offset_max.y), 1};
   blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.srcSubresource.mipLevel = source_region.mip_level;
   blit.srcSubresource.baseArrayLayer = 0;
   blit.srcSubresource.layerCount = 1;
   blit.dstOffsets[0] = {static_cast<int>(target_region.offset_min.x), static_cast<int>(target_region.offset_min.y), 0};
   blit.dstOffsets[1] = {static_cast<int>(target_region.offset_max.x), static_cast<int>(target_region.offset_max.y), 1};
   blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.dstSubresource.mipLevel = target_region.mip_level;
   blit.dstSubresource.baseArrayLayer = 0;
   blit.dstSubresource.layerCount = 1;

   vkCmdBlitImage(m_command_buffer, source_tex.vulkan_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target_tex.vulkan_image(),
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

void CommandList::reset_timestamp_array(const QueryPool& timestamp_array, u32 first, u32 count) const
{
   vkCmdResetQueryPool(m_command_buffer, timestamp_array.vulkan_query_pool(), first, count);
}

void CommandList::write_timestamp(const PipelineStage stage, const QueryPool& timestamp_array, const u32 timestamp_index) const
{
   vkCmdWriteTimestamp(m_command_buffer, vulkan::to_vulkan_pipeline_stage(stage), timestamp_array.vulkan_query_pool(), timestamp_index);
}

void CommandList::push_descriptors(const u32 set_index, DescriptorWriter& writer, const PipelineType pipeline_type) const
{
   const auto writes = writer.vulkan_descriptor_writes();
   vulkan::vkCmdPushDescriptorSetKHR(m_command_buffer, vulkan::to_vulkan_pipeline_bind_point(pipeline_type), m_bound_pipeline_layout,
                                     set_index, static_cast<u32>(writes.size()), writes.data());
}

void CommandList::draw_indexed_indirect_with_count(const Buffer& draw_call_buffer, const Buffer& count_buffer, const u32 max_draw_calls,
                                                   const u32 stride, const u32 count_buffer_offset)
{
   this->handle_pending_descriptors(PipelineType::Graphics);
   vulkan::vkCmdDrawIndexedIndirectCount(m_command_buffer, draw_call_buffer.vulkan_buffer(), 0, count_buffer.vulkan_buffer(),
                                         count_buffer_offset, max_draw_calls, stride);
}

void CommandList::draw_indirect_with_count(const Buffer& draw_call_buffer, const Buffer& count_buffer, const u32 max_draw_calls,
                                           const u32 stride)
{
   this->handle_pending_descriptors(PipelineType::Graphics);
   vkCmdDrawIndirectCount(m_command_buffer, draw_call_buffer.vulkan_buffer(), 0, count_buffer.vulkan_buffer(), 0, max_draw_calls, stride);
}

void CommandList::update_buffer(const Buffer& buffer, const u32 offset, const u32 size, const void* data) const
{
   vkCmdUpdateBuffer(m_command_buffer, buffer.vulkan_buffer(), offset, size, data);
}

void CommandList::begin_rendering(const RenderingInfo& info) const
{
   std::vector<VkRenderingAttachmentInfo> color_attachments;
   color_attachments.resize(info.color_attachments.size());

   for (const auto [index, attachment] : Enumerate(info.color_attachments)) {
      color_attachments[index] = vulkan::to_vulkan_rendering_attachment_info(attachment);
   }

   VkRenderingInfo vulkan_info{VK_STRUCTURE_TYPE_RENDERING_INFO};
   vulkan_info.renderArea.offset.x = info.render_area_offset.x;
   vulkan_info.renderArea.offset.y = info.render_area_offset.y;
   vulkan_info.renderArea.extent.width = info.render_area_extent.x;
   vulkan_info.renderArea.extent.height = info.render_area_extent.y;
   vulkan_info.layerCount = info.layer_count;
   vulkan_info.viewMask = info.view_mask;
   vulkan_info.colorAttachmentCount = static_cast<u32>(color_attachments.size());
   vulkan_info.pColorAttachments = color_attachments.data();

   VkRenderingAttachmentInfo depth_attachment;
   if (info.depth_attachment.has_value()) {
      depth_attachment = vulkan::to_vulkan_rendering_attachment_info(*info.depth_attachment);
      vulkan_info.pDepthAttachment = &depth_attachment;
   }

   vkCmdBeginRendering(m_command_buffer, &vulkan_info);

   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(info.render_area_extent.x);
   viewport.height = static_cast<float>(info.render_area_extent.y);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = VkExtent2D{static_cast<u32>(info.render_area_extent.x), static_cast<u32>(info.render_area_extent.y)};
   vkCmdSetScissor(m_command_buffer, 0, 1, &scissor);
}

void CommandList::end_rendering() const
{
   vkCmdEndRendering(m_command_buffer);
}

void CommandList::bind_raw_uniform_buffer(const u32 binding, const Buffer& buffer)
{
   m_descriptor_writer.set_raw_uniform_buffer(binding, buffer);
   m_has_pending_descriptors = true;
}

void CommandList::bind_storage_buffer(const u32 binding, const Buffer& buffer)
{
   m_descriptor_writer.set_storage_buffer(binding, buffer);
   m_has_pending_descriptors = true;
}

void CommandList::bind_storage_buffer(const u32 binding, const Buffer& buffer, u32 offset, u32 size)
{
   m_descriptor_writer.set_storage_buffer(binding, buffer, offset, size);
   m_has_pending_descriptors = true;
}

void CommandList::bind_acceleration_structure(const u32 binding, const ray_tracing::AccelerationStructure& acc_structure)
{
   m_descriptor_writer.set_acceleration_structure(binding, acc_structure);
   m_has_pending_descriptors = true;
}

void CommandList::set_viewport(const Vector4 dimensions, const float min_depth, const float max_depth) const
{
   VkViewport viewport{};
   viewport.x = dimensions.x;
   viewport.y = dimensions.y;
   viewport.width = dimensions.z;
   viewport.height = dimensions.w;
   viewport.minDepth = min_depth;
   viewport.maxDepth = max_depth;
   vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);
}

void CommandList::trace_rays(const ray_tracing::ShaderBindingTable& binding_table, const glm::ivec3 extent)
{
   this->handle_pending_descriptors(PipelineType::RayTracing);
   vulkan::vkCmdTraceRaysKHR(m_command_buffer, &binding_table.gen_rays_region(), &binding_table.miss_region(), &binding_table.hit_region(),
                             &binding_table.callable_region(), extent.x, extent.y, extent.z);
}

void CommandList::bind_texture_image(const u32 binding, const Texture& texture)
{
   m_descriptor_writer.set_texture_only(binding, texture);
   m_has_pending_descriptors = true;
}

void CommandList::bind_texture_view_image(const u32 binding, const TextureView& texture)
{
   m_descriptor_writer.set_texture_view_only(binding, texture);
   m_has_pending_descriptors = true;
}

void CommandList::bind_texture(const u32 binding, const Texture& texture)
{
   auto& sampler = m_device.sampler_cache().find_sampler(texture.sampler_properties());
   m_descriptor_writer.set_sampled_texture(binding, texture, sampler);
   m_has_pending_descriptors = true;
}

void CommandList::bind_texture_array(const u32 binding, const std::span<const Texture*> textures)
{
   m_descriptor_writer.set_texture_array(binding, textures);
   m_has_pending_descriptors = true;
}

void CommandList::bind_storage_image(const u32 binding, const Texture& texture)
{
   m_descriptor_writer.set_storage_image(binding, texture);
   m_has_pending_descriptors = true;
}

void CommandList::bind_storage_image_view(const u32 binding, const TextureView& texture)
{
   m_descriptor_writer.set_storage_image_view(binding, texture);
   m_has_pending_descriptors = true;
}

void CommandList::begin_query(const QueryPool& query_pool, const u32 query_index) const
{
   vkCmdBeginQuery(m_command_buffer, query_pool.vulkan_query_pool(), query_index, 0);
}

void CommandList::end_query(const QueryPool& query_pool, const u32 query_index) const
{
   vkCmdEndQuery(m_command_buffer, query_pool.vulkan_query_pool(), query_index);
}

WorkTypeFlags CommandList::work_types() const
{
   return m_work_types;
}

uint64_t CommandList::triangle_count() const
{
   return m_triangle_count;
}

void CommandList::set_debug_name(const std::string_view name) const
{
   if (name.empty())
      return;

   VkDebugUtilsObjectNameInfoEXT debug_utils_object_name{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
   debug_utils_object_name.objectHandle = reinterpret_cast<u64>(m_command_buffer);
   debug_utils_object_name.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
   debug_utils_object_name.pObjectName = name.data();
   [[maybe_unused]] const auto result = vulkan::vkSetDebugUtilsObjectNameEXT(m_device.vulkan_device(), &debug_utils_object_name);

   assert(result == VK_SUCCESS);
}

void CommandList::handle_pending_descriptors(const PipelineType pipeline_type)
{
   if (!m_has_pending_descriptors)
      return;

   m_has_pending_descriptors = false;
   this->push_descriptors(0, m_descriptor_writer, pipeline_type);
   m_descriptor_writer.reset_count();
}

}// namespace triglav::graphics_api
