#include "CommandList.h"

#include "DescriptorWriter.h"
#include "Device.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "Texture.h"
#include "TimestampArray.h"
#include "vulkan/Util.h"

#include <cassert>

namespace triglav::graphics_api {

CommandList::CommandList(Device& device, const VkCommandBuffer commandBuffer, const VkCommandPool commandPool,
                         const WorkTypeFlags workTypes) :
    m_device(device),
    m_commandBuffer(commandBuffer),
    m_commandPool(commandPool),
    m_workTypes(workTypes),
    m_descriptorWriter(device),
    m_cmdPushDescriptorSet(
       reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(vkGetDeviceProcAddr(device.vulkan_device(), "vkCmdPushDescriptorSetKHR")))
{
}

CommandList::~CommandList()
{
   if (m_commandBuffer != nullptr) {
      vkFreeCommandBuffers(m_device.vulkan_device(), m_commandPool, 1, &m_commandBuffer);
   }
}

CommandList::CommandList(CommandList&& other) noexcept :
    m_device(other.m_device),
    m_commandBuffer(std::exchange(other.m_commandBuffer, nullptr)),
    m_commandPool(std::exchange(other.m_commandPool, nullptr)),
    m_workTypes(std::exchange(other.m_workTypes, WorkType::None)),
    m_descriptorWriter(std::move(other.m_descriptorWriter)),
    m_cmdPushDescriptorSet(other.m_cmdPushDescriptorSet)
{
}

CommandList& CommandList::operator=(CommandList&& other) noexcept
{
   if (this == &other)
      return *this;
   m_commandBuffer = std::exchange(other.m_commandBuffer, nullptr);
   m_commandPool = std::exchange(other.m_commandPool, nullptr);
   m_workTypes = std::exchange(other.m_workTypes, WorkType::None);
   m_descriptorWriter = std::move(other.m_descriptorWriter);
   m_cmdPushDescriptorSet = other.m_cmdPushDescriptorSet;
   return *this;
}

Status CommandList::begin(const SubmitType type) const
{
   m_triangleCount = 0;

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   if (type == SubmitType::OneTime) {
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   }

   if (vkBeginCommandBuffer(this->vulkan_command_buffer(), &beginInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status CommandList::finish() const
{
   if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

VkCommandBuffer CommandList::vulkan_command_buffer() const
{
   return m_commandBuffer;
}

void CommandList::begin_render_pass(const Framebuffer& framebuffer, std::span<ClearValue> clearValues) const
{
   static constexpr auto maxClearValues{6};

   std::array<VkClearValue, maxClearValues> vulkanClearValues{};
   for (auto i = 0; i < clearValues.size(); ++i) {
      if (std::holds_alternative<Color>(clearValues[i].value)) {
         const auto [r, g, b, a] = std::get<Color>(clearValues[i].value);
         vulkanClearValues[i].color = {{r, g, b, a}};
      } else if (std::holds_alternative<DepthStenctilValue>(clearValues[i].value)) {
         const auto [depthValue, stencilValue] = std::get<DepthStenctilValue>(clearValues[i].value);
         vulkanClearValues[i].depthStencil.depth = depthValue;
         vulkanClearValues[i].depthStencil.stencil = stencilValue;
      }
   }

   const auto [width, height] = framebuffer.resolution();

   VkRenderPassBeginInfo renderPassInfo{};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass = framebuffer.vulkan_render_pass();
   renderPassInfo.framebuffer = framebuffer.vulkan_framebuffer();
   renderPassInfo.renderArea.offset = {0, 0};
   renderPassInfo.renderArea.extent.width = width;
   renderPassInfo.renderArea.extent.height = height;
   renderPassInfo.clearValueCount = clearValues.size();
   renderPassInfo.pClearValues = vulkanClearValues.data();

   vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

   VkViewport viewport{};
   viewport.x = 0.0f;
   viewport.y = 0.0f;
   viewport.width = static_cast<float>(width);
   viewport.height = static_cast<float>(height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = VkExtent2D{width, height};
   vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

void CommandList::end_render_pass() const
{
   vkCmdEndRenderPass(m_commandBuffer);
}

void CommandList::bind_pipeline(const Pipeline& pipeline)
{
   vkCmdBindPipeline(m_commandBuffer, vulkan::to_vulkan_pipeline_bind_point(pipeline.pipeline_type()), pipeline.vulkan_pipeline());
   m_boundPipelineLayout = *pipeline.layout();
}

void CommandList::bind_descriptor_set(const DescriptorView& descriptorSet) const
{
   const auto vulkanDescriptorSet = descriptorSet.vulkan_descriptor_set();
   vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_boundPipelineLayout, 0, 1, &vulkanDescriptorSet, 0, nullptr);
}

void CommandList::draw_primitives(int vertexCount, int vertexOffset, int instanceCount, int firstInstance)
{
   if (m_hasPendingDescriptors) {
      m_hasPendingDescriptors = false;
      this->push_descriptors(0, m_descriptorWriter, PipelineType::Graphics);
      m_descriptorWriter.reset_count();
   }

   m_triangleCount += instanceCount * (vertexCount / 3);
   vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, vertexOffset, firstInstance);
}

void CommandList::draw_primitives(const int vertexCount, const int vertexOffset)
{
   this->draw_primitives(vertexCount, vertexOffset, 1, 0);
}

void CommandList::draw_indexed_primitives(const int indexCount, const int indexOffset, const int vertexOffset)
{
   if (m_hasPendingDescriptors) {
      m_hasPendingDescriptors = false;
      this->push_descriptors(0, m_descriptorWriter, PipelineType::Graphics);
      m_descriptorWriter.reset_count();
   }

   m_triangleCount += indexCount / 3;
   vkCmdDrawIndexed(m_commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
}

void CommandList::dispatch(u32 x, u32 y, u32 z)
{
   // For some reason I need that XDDD
   assert(x != 0 && y != 0 && z != 0);

   if (m_hasPendingDescriptors) {
      m_hasPendingDescriptors = false;
      this->push_descriptors(0, m_descriptorWriter, PipelineType::Compute);
      m_descriptorWriter.reset_count();
   }

   vkCmdDispatch(m_commandBuffer, x, y, z);
}

void CommandList::bind_vertex_buffer(const Buffer& buffer, uint32_t layoutIndex) const
{
   const std::array buffers{buffer.vulkan_buffer()};
   constexpr std::array<VkDeviceSize, 1> offsets{0};
   vkCmdBindVertexBuffers(m_commandBuffer, layoutIndex, buffers.size(), buffers.data(), offsets.data());
}

void CommandList::bind_index_buffer(const Buffer& buffer) const
{
   vkCmdBindIndexBuffer(m_commandBuffer, buffer.vulkan_buffer(), 0, VK_INDEX_TYPE_UINT32);
}

void CommandList::copy_buffer(const Buffer& source, const Buffer& dest) const
{
   VkBufferCopy region{};
   region.size = source.size();
   vkCmdCopyBuffer(m_commandBuffer, source.vulkan_buffer(), dest.vulkan_buffer(), 1, &region);
}

void CommandList::copy_buffer(const Buffer& source, const Buffer& dest, u32 srcOffset, u32 dstOffset, u32 size) const
{
   assert(size > 0);

   VkBufferCopy region{};
   region.srcOffset = srcOffset;
   region.dstOffset = dstOffset;
   region.size = size;
   vkCmdCopyBuffer(m_commandBuffer, source.vulkan_buffer(), dest.vulkan_buffer(), 1, &region);
}

void CommandList::copy_buffer_to_texture(const Buffer& source, const Texture& destination, const int mipLevel) const
{
   VkBufferImageCopy region{};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;
   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel = mipLevel;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;
   region.imageOffset = {0, 0, 0};
   region.imageExtent = {destination.width(), destination.height(), 1};
   vkCmdCopyBufferToImage(m_commandBuffer, source.vulkan_buffer(), destination.vulkan_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                          &region);
}

void CommandList::copy_texture(const Texture& source, const TextureState srcState, const Texture& destination, const TextureState dstState)
{
   VkImageCopy imageCopy{.srcSubresource{
                            .aspectMask = vulkan::to_vulkan_aspect_flags(source.usage_flags()),
                            .mipLevel = 0,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                         },
                         .srcOffset{0, 0, 0},
                         .dstSubresource{
                            .aspectMask = vulkan::to_vulkan_aspect_flags(destination.usage_flags()),
                            .mipLevel = 0,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                         },
                         .dstOffset{0, 0, 0},
                         .extent{destination.width(), destination.height(), 1}};
   vkCmdCopyImage(m_commandBuffer, source.vulkan_image(), vulkan::to_vulkan_image_layout(srcState), destination.vulkan_image(),
                  vulkan::to_vulkan_image_layout(dstState), 1, &imageCopy);
}

void CommandList::push_constant_ptr(const PipelineStage stage, const void* ptr, const size_t size, const size_t offset) const
{
   assert(m_boundPipelineLayout != nullptr);
   vkCmdPushConstants(m_commandBuffer, m_boundPipelineLayout, vulkan::to_vulkan_shader_stage_flags(stage), offset, size, ptr);
}

void CommandList::texture_barrier(const PipelineStageFlags sourceStage, const PipelineStageFlags targetStage,
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
      barrier.oldLayout = vulkan::to_vulkan_image_layout(info.sourceState);
      barrier.newLayout = vulkan::to_vulkan_image_layout(info.targetState);
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = info.texture->vulkan_image();
      barrier.subresourceRange.aspectMask = vulkan::to_vulkan_aspect_flags(info.texture->usage_flags());
      barrier.subresourceRange.baseMipLevel = info.baseMipLevel;
      barrier.subresourceRange.levelCount = info.mipLevelCount;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = 1;
      barrier.srcAccessMask = vulkan::to_vulkan_access_flags(info.sourceState);
      barrier.dstAccessMask = vulkan::to_vulkan_access_flags(info.targetState);
   }

   vkCmdPipelineBarrier(m_commandBuffer, vulkan::to_vulkan_pipeline_stage_flags(sourceStage),
                        vulkan::to_vulkan_pipeline_stage_flags(targetStage), 0, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
}

void CommandList::texture_barrier(const PipelineStageFlags sourceStage, const PipelineStageFlags targetStage,
                                  const TextureBarrierInfo& info) const
{
   this->texture_barrier(sourceStage, targetStage, std::span(&info, &info + 1));
}

void CommandList::execution_barrier(PipelineStageFlags sourceStage, PipelineStageFlags targetStage) const
{
   vkCmdPipelineBarrier(m_commandBuffer, vulkan::to_vulkan_pipeline_stage_flags(sourceStage),
                        vulkan::to_vulkan_pipeline_stage_flags(targetStage), 0, 0, nullptr, 0, nullptr, 0, nullptr);
}

void CommandList::blit_texture(const Texture& sourceTex, const TextureRegion& sourceRegion, const Texture& targetTex,
                               const TextureRegion& targetRegion) const
{
   VkImageBlit blit{};
   blit.srcOffsets[0] = {static_cast<int>(sourceRegion.offsetMin.x), static_cast<int>(sourceRegion.offsetMin.y), 0};
   blit.srcOffsets[1] = {static_cast<int>(sourceRegion.offsetMax.x), static_cast<int>(sourceRegion.offsetMax.y), 1};
   blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.srcSubresource.mipLevel = sourceRegion.mipLevel;
   blit.srcSubresource.baseArrayLayer = 0;
   blit.srcSubresource.layerCount = 1;
   blit.dstOffsets[0] = {static_cast<int>(targetRegion.offsetMin.x), static_cast<int>(targetRegion.offsetMin.y), 0};
   blit.dstOffsets[1] = {static_cast<int>(targetRegion.offsetMax.x), static_cast<int>(targetRegion.offsetMax.y), 1};
   blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   blit.dstSubresource.mipLevel = targetRegion.mipLevel;
   blit.dstSubresource.baseArrayLayer = 0;
   blit.dstSubresource.layerCount = 1;

   vkCmdBlitImage(m_commandBuffer, sourceTex.vulkan_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetTex.vulkan_image(),
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
}

void CommandList::reset_timestamp_array(const TimestampArray& timestampArray, u32 first, u32 count) const
{
   vkCmdResetQueryPool(m_commandBuffer, timestampArray.vulkan_query_pool(), first, count);
}

void CommandList::write_timestamp(const PipelineStage stage, const TimestampArray& timestampArray, const u32 timestampIndex) const
{
   vkCmdWriteTimestamp(m_commandBuffer, vulkan::to_vulkan_pipeline_stage(stage), timestampArray.vulkan_query_pool(), timestampIndex);
}

void CommandList::push_descriptors(const u32 setIndex, DescriptorWriter& writer, const PipelineType pipelineType) const
{
   const auto writes = writer.vulkan_descriptor_writes();
   assert(m_cmdPushDescriptorSet);
   m_cmdPushDescriptorSet(m_commandBuffer, vulkan::to_vulkan_pipeline_bind_point(pipelineType), m_boundPipelineLayout, setIndex,
                          static_cast<u32>(writes.size()), writes.data());
}

WorkTypeFlags CommandList::work_types() const
{
   return m_workTypes;
}

uint64_t CommandList::triangle_count() const
{
   return m_triangleCount;
}

Status CommandList::reset() const
{
   m_triangleCount = 0;
   if (vkResetCommandBuffer(m_commandBuffer, 0) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
}

void CommandList::bind_storage_buffer(u32 binding, const Buffer& buffer)
{
   m_descriptorWriter.set_storage_buffer(binding, buffer);
   m_hasPendingDescriptors = true;
}

void CommandList::bind_storage_buffer(u32 binding, const Buffer& buffer, u32 offset, u32 size)
{
   m_descriptorWriter.set_storage_buffer(binding, buffer, offset, size);
   m_hasPendingDescriptors = true;
}

void CommandList::bind_raw_uniform_buffer(u32 binding, const Buffer& buffer)
{
   m_descriptorWriter.set_raw_uniform_buffer(binding, buffer);
   m_hasPendingDescriptors = true;
}

void CommandList::bind_texture(u32 binding, const Texture& texture)
{
   auto& sampler = m_device.sampler_cache().find_sampler(texture.sampler_properties());
   m_descriptorWriter.set_sampled_texture(binding, texture, sampler);
   m_hasPendingDescriptors = true;
}

}// namespace triglav::graphics_api
