#include "CommandList.h"

#include "Framebuffer.h"
#include "Pipeline.h"
#include "Texture.h"
#include "vulkan/Util.h"

namespace graphics_api {

CommandList::CommandList(const VkCommandBuffer commandBuffer, const VkDevice device,
                         const VkCommandPool commandPool) :
    m_commandBuffer(commandBuffer),
    m_device(device),
    m_commandPool(commandPool)
{
}

CommandList::~CommandList()
{
   if (m_commandBuffer != nullptr) {
      vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
   }
}

CommandList::CommandList(CommandList &&other) noexcept :
    m_commandBuffer(std::exchange(other.m_commandBuffer, nullptr)),
    m_device(std::exchange(other.m_device, nullptr)),
    m_commandPool(std::exchange(other.m_commandPool, nullptr))
{
}

CommandList &CommandList::operator=(CommandList &&other) noexcept
{
   if (this == &other)
      return *this;
   m_commandBuffer = std::exchange(other.m_commandBuffer, nullptr);
   m_device        = std::exchange(other.m_device, nullptr);
   m_commandPool   = std::exchange(other.m_commandPool, nullptr);
   return *this;
}

Status CommandList::begin_one_time() const
{
   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   if (vkBeginCommandBuffer(this->vulkan_command_buffer(), &beginInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status CommandList::finish_one_time() const
{
   if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   return Status::Success;
}

Status CommandList::begin_graphic(const Framebuffer &framebuffer, const Color &clearColor) const
{
   vkResetCommandBuffer(m_commandBuffer, 0);

   VkCommandBufferBeginInfo beginInfo{};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }

   std::array<VkClearValue, 3> clearValues{};
   clearValues[0].color = {
           {clearColor.r, clearColor.g, clearColor.b, clearColor.a}
   };
   clearValues[1].depthStencil.depth   = 1.0f;
   clearValues[1].depthStencil.stencil = 0.0f;
   clearValues[2].color                = clearValues[0].color;

   const auto [width, height] = framebuffer.resolution();

   VkRenderPassBeginInfo renderPassInfo{};
   renderPassInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass               = framebuffer.vulkan_render_pass();
   renderPassInfo.framebuffer              = framebuffer.vulkan_framebuffer();
   renderPassInfo.renderArea.offset        = {0, 0};
   renderPassInfo.renderArea.extent.width  = width;
   renderPassInfo.renderArea.extent.height = height;
   renderPassInfo.clearValueCount          = clearValues.size();
   renderPassInfo.pClearValues             = clearValues.data();

   vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

   VkViewport viewport{};
   viewport.x        = 0.0f;
   viewport.y        = 0.0f;
   viewport.width    = static_cast<float>(width);
   viewport.height   = static_cast<float>(height);
   viewport.minDepth = 0.0f;
   viewport.maxDepth = 1.0f;
   vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

   VkRect2D scissor{};
   scissor.offset = {0, 0};
   scissor.extent = VkExtent2D{width, height};
   vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

   return Status::Success;
}

Status CommandList::finish_graphic() const
{
   vkCmdEndRenderPass(m_commandBuffer);
   return this->finish_one_time();
}

VkCommandBuffer CommandList::vulkan_command_buffer() const
{
   return m_commandBuffer;
}

void CommandList::bind_pipeline(const Pipeline &pipeline)
{
   vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vulkan_pipeline());
   m_boundPipelineLayout = *pipeline.layout();
}

void CommandList::bind_descriptor_set(const DescriptorView &descriptorSet) const
{
   const auto vulkanDescriptorSet = descriptorSet.vulkan_descriptor_set();
   vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           descriptorSet.vulkan_pipeline_layout(), 0, 1, &vulkanDescriptorSet, 0, nullptr);
}

void CommandList::draw_primitives(const int vertexCount, const int vertexOffset) const
{
   vkCmdDraw(m_commandBuffer, vertexCount, 1, vertexOffset, 0);
}

void CommandList::draw_indexed_primitives(const int indexCount, const int indexOffset,
                                          const int vertexOffset) const
{
   vkCmdDrawIndexed(m_commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0);
}

void CommandList::bind_vertex_buffer(const Buffer &buffer, uint32_t layoutIndex) const
{
   const std::array buffers{buffer.vulkan_buffer()};
   constexpr std::array<VkDeviceSize, 1> offsets{0};
   vkCmdBindVertexBuffers(m_commandBuffer, layoutIndex, buffers.size(), buffers.data(), offsets.data());
}

void CommandList::bind_index_buffer(const Buffer &buffer) const
{
   vkCmdBindIndexBuffer(m_commandBuffer, buffer.vulkan_buffer(), 0, VK_INDEX_TYPE_UINT32);
}

void CommandList::copy_buffer(const Buffer &source, const Buffer &dest)
{
   VkBufferCopy region{};
   region.size = source.size();
   vkCmdCopyBuffer(m_commandBuffer, source.vulkan_buffer(), dest.vulkan_buffer(), 1, &region);
}

void CommandList::copy_buffer_to_texture(const Buffer &source, const Texture &destination)
{
   VkImageMemoryBarrier writeBarrier{};
   writeBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   writeBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
   writeBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   writeBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
   writeBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
   writeBarrier.image                           = destination.vulkan_image();
   writeBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
   writeBarrier.subresourceRange.baseMipLevel   = 0;
   writeBarrier.subresourceRange.levelCount     = 1;
   writeBarrier.subresourceRange.baseArrayLayer = 0;
   writeBarrier.subresourceRange.layerCount     = 1;
   writeBarrier.srcAccessMask                   = 0;
   writeBarrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

   vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                        0, nullptr, 0, nullptr, 1, &writeBarrier);

   VkBufferImageCopy region{};
   region.bufferOffset                    = 0;
   region.bufferRowLength                 = 0;
   region.bufferImageHeight               = 0;
   region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel       = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount     = 1;
   region.imageOffset                     = {0, 0, 0};
   region.imageExtent                     = {destination.width(), destination.height(), 1};
   vkCmdCopyBufferToImage(m_commandBuffer, source.vulkan_buffer(), destination.vulkan_image(),
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

   VkImageMemoryBarrier fragmentShaderBarrier{};
   fragmentShaderBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   fragmentShaderBarrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   fragmentShaderBarrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   fragmentShaderBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
   fragmentShaderBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
   fragmentShaderBarrier.image                           = destination.vulkan_image();
   fragmentShaderBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
   fragmentShaderBarrier.subresourceRange.baseMipLevel   = 0;
   fragmentShaderBarrier.subresourceRange.levelCount     = 1;
   fragmentShaderBarrier.subresourceRange.baseArrayLayer = 0;
   fragmentShaderBarrier.subresourceRange.layerCount     = 1;
   fragmentShaderBarrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
   fragmentShaderBarrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;

   vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                        &fragmentShaderBarrier);
}

void CommandList::push_constant_ptr(const ShaderStage stage, const void *ptr, const size_t size,
                                    const size_t offset) const
{
   assert(m_boundPipelineLayout != nullptr);
   vkCmdPushConstants(m_commandBuffer, m_boundPipelineLayout,
                      vulkan::to_vulkan_shader_stage_flags(ShaderStage::None | stage), offset, size, ptr);
}

Status CommandList::reset()
{
   if (vkResetCommandBuffer(m_commandBuffer, 0) != VK_SUCCESS) {
      return Status::UnsupportedDevice;
   }
   return Status::Success;
}

}// namespace graphics_api
