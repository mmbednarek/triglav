#include "BuildContext.hpp"

#include "triglav/Name.hpp"
#include "triglav/NameResolution.hpp"
#include "triglav/Ranges.hpp"
#include "triglav/graphics_api/CommandList.hpp"
#include "triglav/graphics_api/GraphicsApi.hpp"
#include "triglav/resource/ResourceManager.hpp"

#include <cstring>
#include <fmt/core.h>

namespace triglav::render_core {

namespace gapi = graphics_api;

BuildContext::BuildContext(graphics_api::Device& device, resource::ResourceManager& resourceManager, const Vector2i screenSize) :
    m_device(device),
    m_resourceManager(resourceManager),
    m_screenSize(screenSize)
{
}

void BuildContext::declare_texture(const Name texName, const Vector2i texDims, gapi::ColorFormat texFormat)
{
   this->add_declaration<detail::decl::Texture>(texName, texDims, texFormat);
}

void BuildContext::declare_render_target(const Name rtName, gapi::ColorFormat rtFormat)
{
   this->m_renderTargets.emplace(rtName, detail::RenderTarget{gapi::ClearValue::color(gapi::ColorPalette::Black),
                                                              gapi::AttachmentAttribute::Color | gapi::AttachmentAttribute::ClearImage |
                                                                 gapi::AttachmentAttribute::StoreImage});
   this->add_declaration<detail::decl::Texture>(rtName, Vector2i{0, 0}, rtFormat, gapi::TextureUsage::ColorAttachment);
}

void BuildContext::declare_sized_render_target(const Name rtName, const Vector2i rtDims, gapi::ColorFormat rtFormat)
{
   this->m_renderTargets.emplace(rtName, detail::RenderTarget{gapi::ClearValue::color(gapi::ColorPalette::Black),
                                                              gapi::AttachmentAttribute::Color | gapi::AttachmentAttribute::ClearImage |
                                                                 gapi::AttachmentAttribute::StoreImage});
   this->add_declaration<detail::decl::Texture>(rtName, rtDims, rtFormat, gapi::TextureUsage::ColorAttachment);
}

void BuildContext::declare_sized_depth_target(Name dtName, Vector2i dtDims, graphics_api::ColorFormat rtFormat)
{
   this->m_renderTargets.emplace(dtName, detail::RenderTarget{gapi::ClearValue::depth_stencil(1.0f, 0),
                                                              gapi::AttachmentAttribute::Depth | gapi::AttachmentAttribute::ClearImage});
   this->add_declaration<detail::decl::Texture>(dtName, dtDims, rtFormat, gapi::TextureUsage::DepthStencilAttachment);
}

void BuildContext::declare_buffer(const Name buffName, const MemorySize size)
{
   this->add_declaration<detail::decl::Buffer>(buffName, size, gapi::BufferUsage::None);
}

void BuildContext::declare_staging_buffer(const Name buffName, const MemorySize size)
{
   this->add_declaration<detail::decl::Buffer>(buffName, size, gapi::BufferUsage::HostVisible);
}

void BuildContext::bind_vertex_shader(VertexShaderName vsName)
{
   m_workTypes |= gapi::WorkType::Graphics;
   m_graphicPipelineState.vertexShader = vsName;
   m_activePipelineStage = gapi::PipelineStage::VertexShader;
}

void BuildContext::bind_fragment_shader(FragmentShaderName fsName)
{
   m_workTypes |= gapi::WorkType::Graphics;
   m_graphicPipelineState.fragmentShader = fsName;
   m_activePipelineStage = gapi::PipelineStage::FragmentShader;
}

void BuildContext::bind_compute_shader(ComputeShaderName csName)
{
   m_workTypes |= gapi::WorkType::Compute;
   m_computePipelineState.computeShader = csName;
   m_activePipelineStage = gapi::PipelineStage::ComputeShader;
}

void BuildContext::bind_rw_texture(const BindingIndex index, const TextureRef texRef)
{
   this->prepare_texture(texRef, gapi::TextureState::General, gapi::TextureUsage::Storage);

   ++m_descriptorCounts.storageTextureCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStage;
   descriptor.descriptorType = gapi::DescriptorType::StorageImage;
   this->set_pipeline_state_descriptor(m_activePipelineStage, index, descriptor);

   this->set_descriptor<detail::descriptor::RWTexture>(index, texRef);
}

void BuildContext::bind_samplable_texture(const BindingIndex index, const TextureRef texRef)
{
   this->prepare_texture(texRef, gapi::TextureState::ShaderRead, gapi::TextureUsage::Sampled);

   ++m_descriptorCounts.samplableTextureCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStage;
   descriptor.descriptorType = gapi::DescriptorType::ImageSampler;
   this->set_pipeline_state_descriptor(m_activePipelineStage, index, descriptor);

   this->set_descriptor<detail::descriptor::SamplableTexture>(index, texRef);
}

void BuildContext::bind_uniform_buffer(const BindingIndex index, const BufferRef buffRef)
{
   this->prepare_buffer(buffRef, gapi::BufferAccess::UniformRead, gapi::BufferUsage::UniformBuffer);

   ++m_descriptorCounts.uniformBufferCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStage;
   descriptor.descriptorType = gapi::DescriptorType::UniformBuffer;
   this->set_pipeline_state_descriptor(m_activePipelineStage, index, descriptor);

   this->set_descriptor<detail::descriptor::UniformBuffer>(index, buffRef);
}

void BuildContext::bind_uniform_buffers(const BindingIndex index, const std::span<const BufferRef> buffers)
{
   for (const auto& buffer : buffers) {
      this->prepare_buffer(buffer, gapi::BufferAccess::UniformRead, gapi::BufferUsage::UniformBuffer);
   }

   m_descriptorCounts.uniformBufferCount += buffers.size();

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStage;
   descriptor.descriptorType = gapi::DescriptorType::UniformBuffer;
   descriptor.descriptorCount = buffers.size();
   this->set_pipeline_state_descriptor(m_activePipelineStage, index, descriptor);

   std::vector<BufferRef> bufferRefs;
   bufferRefs.reserve(buffers.size());
   std::ranges::copy(buffers, std::back_inserter(bufferRefs));
   this->set_descriptor<detail::descriptor::UniformBufferArray>(index, std::move(bufferRefs));
}

void BuildContext::bind_storage_buffer(const BindingIndex index, const BufferRef buffRef)
{
   this->prepare_buffer(buffRef, gapi::BufferAccess::ShaderRead, gapi::BufferUsage::StorageBuffer);

   ++m_descriptorCounts.storageBufferCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStage;
   descriptor.descriptorType = gapi::DescriptorType::StorageBuffer;
   this->set_pipeline_state_descriptor(m_activePipelineStage, index, descriptor);

   this->set_descriptor<detail::descriptor::StorageBuffer>(index, buffRef);
}

void BuildContext::bind_vertex_layout(const VertexLayout& layout)
{
   m_graphicPipelineState.vertexLayout = layout;
}

void BuildContext::bind_vertex_buffer(const Name buffName)
{
   this->setup_buffer_barrier(buffName, gapi::BufferAccess::VertexRead, gapi::PipelineStage::VertexInput);

   this->add_buffer_flag(buffName, gapi::BufferUsage::VertexBuffer);
   this->add_command<detail::cmd::BindVertexBuffer>(buffName);
}

void BuildContext::bind_index_buffer(const Name buffName)
{
   this->setup_buffer_barrier(buffName, gapi::BufferAccess::IndexRead, gapi::PipelineStage::VertexInput);

   this->add_buffer_flag(buffName, gapi::BufferUsage::IndexBuffer);
   this->add_command<detail::cmd::BindIndexBuffer>(buffName);
}

void BuildContext::begin_render_pass_raw(const Name passName, const std::span<Name> renderTargets)
{
   for (const auto rtName : renderTargets) {
      const auto& rtTexture = this->declaration<detail::decl::Texture>(rtName);
      const auto& renderTarget = m_renderTargets.at(rtName);
      const auto isDepthTarget = renderTarget.flags & gapi::AttachmentAttribute::Depth;

      const auto targetStage = isDepthTarget ? gapi::PipelineStage::EarlyZ : gapi::PipelineStage::AttachmentOutput;
      const auto lastUsedStage = isDepthTarget ? gapi::PipelineStage::LateZ : gapi::PipelineStage::AttachmentOutput;
      this->setup_texture_barrier(rtName, gapi::TextureState::RenderTarget, targetStage, lastUsedStage);

      if (isDepthTarget) {
         m_graphicPipelineState.depthTargetFormat = rtTexture.texFormat;
      } else {
         m_graphicPipelineState.renderTargetFormats.emplace_back(rtTexture.texFormat);
      }
   }

   std::vector<Name> renderTargetNames(renderTargets.size());
   std::ranges::copy(renderTargets, renderTargetNames.begin());
   this->add_command<detail::cmd::BeginRenderPass>(passName, std::move(renderTargetNames));

   m_isWithinRenderPass = true;
}

void BuildContext::end_render_pass()
{
   m_graphicPipelineState.depthTargetFormat.reset();
   m_graphicPipelineState.renderTargetFormats.clear();
   this->add_command<detail::cmd::EndRenderPass>();

   m_isWithinRenderPass = false;
}

void BuildContext::handle_descriptor_bindings()
{
   if (m_descriptors.empty()) {
      return;
   }

   for (const auto& descriptor : m_descriptors) {
      assert(descriptor.has_value());
   }

   this->add_command<detail::cmd::BindDescriptors>(std::move(m_descriptors));
}

graphics_api::DescriptorArray& BuildContext::allocate_descriptors(DescriptorStorage& storage, graphics_api::DescriptorPool& pool) const
{
   graphics_api::DescriptorLayoutArray descriptorLayouts;
   descriptorLayouts.add_from_pipeline(*m_currentPipeline);

   return storage.store_descriptor_array(GAPI_CHECK(pool.allocate_array(descriptorLayouts)));
}

void BuildContext::write_descriptor(ResourceStorage& storage, const graphics_api::DescriptorView& descView,
                                    const detail::cmd::BindDescriptors& descriptors, const u32 frameIndex) const
{
   graphics_api::DescriptorWriter writer(m_device, descView);

   for (const auto [index, descVariant] : Enumerate(descriptors.descriptors)) {
      std::visit(
         [this, index, frameIndex, &writer, &storage]<typename TDescriptor>(const TDescriptor& desc) {
            if constexpr (std::is_same_v<TDescriptor, detail::descriptor::RWTexture>) {
               writer.set_storage_image(index, this->resolve_texture_ref(storage, desc.texRef, frameIndex));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SamplableTexture>) {
               const gapi::Texture& texture = this->resolve_texture_ref(storage, desc.texRef, frameIndex);
               writer.set_sampled_texture(index, texture, m_device.sampler_cache().find_sampler(texture.sampler_properties()));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBuffer>) {
               writer.set_raw_uniform_buffer(index, this->resolve_buffer_ref(storage, desc.buffRef, frameIndex));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBufferArray>) {
               std::vector<const gapi::Buffer*> buffers;
               buffers.reserve(desc.buffers.size());
               for (const auto ref : desc.buffers) {
                  buffers.emplace_back(&this->resolve_buffer_ref(storage, ref, frameIndex));
               }
               writer.set_uniform_buffer_array(index, buffers);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::StorageBuffer>) {
               writer.set_storage_buffer(index, this->resolve_buffer_ref(storage, desc.buffRef, frameIndex));
            }
         },
         *descVariant);
   }
}

void BuildContext::add_texture_flag(const Name texName, const graphics_api::TextureUsage flag)
{
   this->declaration<detail::decl::Texture>(texName).texUsageFlags |= flag;
}

void BuildContext::add_buffer_flag(const Name buffName, const graphics_api::BufferUsage flag)
{
   std::get<detail::decl::Buffer>(m_declarations[buffName]).buffUsageFlags |= flag;
}

void BuildContext::setup_texture_barrier(const Name texName, const graphics_api::TextureState targetState,
                                         const graphics_api::PipelineStage targetStage,
                                         const std::optional<graphics_api::PipelineStage> lastUsedStage)
{
   auto& tex = this->declaration<detail::decl::Texture>(texName);

   const auto lateStage = lastUsedStage.value_or(targetStage);

   const auto targetMemAccess = gapi::to_memory_access(targetState);
   if (targetMemAccess == gapi::MemoryAccess::Write || tex.currentState != targetState || tex.lastTextureBarrier == nullptr) {
      if (tex.lastStages != 0) {
         auto barrier = std::make_unique<TextureBarrier>(texName, tex.lastStages, targetStage, tex.currentState, targetState);
         tex.lastTextureBarrier = this->add_command_before_render_pass<detail::cmd::PlaceTextureBarrier>(std::move(barrier)).barrier.get();
      }
      tex.lastStages = lateStage;
   } else if (tex.lastTextureBarrier != nullptr) {
      tex.lastTextureBarrier->dstStageFlags |= targetStage;
      tex.lastStages |= lateStage;
   }

   tex.currentState = targetState;
}

void BuildContext::setup_buffer_barrier(const Name buffName, const gapi::BufferAccess targetAccess, const gapi::PipelineStage targetStage)
{
   auto& buffer = this->declaration<detail::decl::Buffer>(buffName);

   const auto targetMemAccess = gapi::to_memory_access(targetAccess);

   if (targetMemAccess == gapi::MemoryAccess::Write || buffer.lastBufferBarrier == nullptr) {
      if (buffer.lastStages != 0) {
         auto barrier = std::make_unique<BufferBarrier>(buffName, buffer.lastStages, targetStage, buffer.currentAccess, targetAccess);
         buffer.lastBufferBarrier = this->add_command_before_render_pass<detail::cmd::PlaceBufferBarrier>(std::move(barrier)).barrier.get();
      }
      buffer.lastStages = targetStage;
   } else if (buffer.lastBufferBarrier != nullptr) {
      buffer.lastBufferBarrier->dstStageFlags |= targetStage;
      buffer.lastBufferBarrier->dstBufferAccess |= targetAccess;
      buffer.lastStages |= targetStage;
   }

   buffer.currentAccess = targetAccess;
}

gapi::RenderingInfo BuildContext::create_rendering_info(ResourceStorage& storage, const detail::cmd::BeginRenderPass& beginRenderPass,
                                                        const u32 frameIndex) const
{
   gapi::RenderingInfo info{};
   gapi::Resolution resolution{};

   for (const auto rtName : beginRenderPass.renderTargets) {
      const auto& renderTarget = m_renderTargets.at(rtName);

      gapi::RenderAttachment attachment{};
      attachment.texture = &storage.texture(rtName, frameIndex);
      attachment.state = gapi::TextureState::RenderTarget;
      attachment.clearValue = renderTarget.clearValue;
      attachment.flags = renderTarget.flags;

      resolution = attachment.texture->resolution();

      if (renderTarget.flags & gapi::AttachmentAttribute::Depth) {
         info.depthAttachment = attachment;
      } else {
         info.colorAttachments.emplace_back(attachment);
      }
   }

   info.layerCount = 1;
   info.renderAreaOffset = {0, 0};
   info.renderAreaExtent = {resolution.width, resolution.height};

   return info;
}

gapi::Texture& BuildContext::resolve_texture_ref(ResourceStorage& storage, TextureRef texRef, u32 frameIndex) const
{
   return std::visit(
      [this, frameIndex, &storage]<typename TVariant>(const TVariant& var) -> gapi::Texture& {
         if constexpr (std::is_same_v<TVariant, Name>) {
            return storage.texture(var, frameIndex);
         } else if constexpr (std::is_same_v<TVariant, FromLastFrame>) {
            return storage.texture(var.name, (FRAMES_IN_FLIGHT_COUNT + frameIndex - 1) % FRAMES_IN_FLIGHT_COUNT);
         } else if constexpr (std::is_same_v<TVariant, TextureName>) {
            return m_resourceManager.get(var);
         } else {
            assert(0);
         }
      },
      texRef);
}

graphics_api::Buffer& BuildContext::resolve_buffer_ref(ResourceStorage& storage, BufferRef buffRef, u32 frameIndex) const
{
   return std::visit(
      [frameIndex, &storage]<typename TVariant>(const TVariant& var) -> gapi::Buffer& {
         if constexpr (std::is_same_v<TVariant, Name>) {
            return storage.buffer(var, frameIndex);
         } else if constexpr (std::is_same_v<TVariant, FromLastFrame>) {
            return storage.buffer(var.name, (FRAMES_IN_FLIGHT_COUNT + frameIndex - 1) % FRAMES_IN_FLIGHT_COUNT);
         } else if constexpr (std::is_same_v<TVariant, gapi::Buffer*>) {
            return *var;
         } else {
            assert(0);
         }
      },
      buffRef);
}

void BuildContext::handle_pending_graphic_state()
{
   this->add_command<detail::cmd::BindGraphicsPipeline>(m_graphicPipelineState);
   m_graphicPipelineState.descriptorState.descriptorCount = 0;
   m_graphicPipelineState.vertexLayout.stride = 0;
   m_graphicPipelineState.vertexLayout.attributes.clear();
   ++m_descriptorCounts.totalDescriptorSets;

   this->handle_descriptor_bindings();
}

void BuildContext::prepare_texture(const TextureRef texRef, const gapi::TextureState state, const gapi::TextureUsage usage)
{
   if (!std::holds_alternative<Name>(texRef) && !std::holds_alternative<FromLastFrame>(texRef)) {
      return;
   }

   const Name texName = std::holds_alternative<Name>(texRef) ? std::get<Name>(texRef) : std::get<FromLastFrame>(texRef).name;

   this->setup_texture_barrier(texName, state, m_activePipelineStage);
   this->add_texture_flag(texName, usage);

   if (gapi::to_memory_access(state) == gapi::MemoryAccess::Read && m_renderTargets.contains(texName)) {
      auto& renderTarget = m_renderTargets.at(texName);
      renderTarget.flags |= gapi::AttachmentAttribute::StoreImage;
   }
}

void BuildContext::prepare_buffer(const BufferRef buffRef, const gapi::BufferAccess access, const gapi::BufferUsage usage)
{
   if (!std::holds_alternative<Name>(buffRef) && !std::holds_alternative<FromLastFrame>(buffRef)) {
      return;
   }

   const Name buffName = std::holds_alternative<Name>(buffRef) ? std::get<Name>(buffRef) : std::get<FromLastFrame>(buffRef).name;

   this->setup_buffer_barrier(buffName, access, m_activePipelineStage);
   this->add_buffer_flag(buffName, usage);
}

u32 BuildContext::flag_variation_count() const
{
   return 1u << m_flags.size();
}

void BuildContext::set_vertex_topology(const graphics_api::VertexTopology topology)
{
   m_graphicPipelineState.vertexTopology = topology;
}

void BuildContext::draw_primitives(const u32 vertexCount, const u32 vertexOffset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawPrimitives>(vertexCount, vertexOffset, 1, 0);
}

void BuildContext::draw_indexed_primitives(const u32 indexCount, const u32 indexOffset, const u32 vertexOffset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedPrimitives>(indexCount, indexOffset, vertexOffset, 1u, 0u);
}

void BuildContext::draw_indexed_primitives(u32 indexCount, u32 indexOffset, u32 vertexOffset, u32 instanceCount, u32 instanceOffset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedPrimitives>(indexCount, indexOffset, vertexOffset, instanceCount, instanceOffset);
}

void BuildContext::draw_full_screen_quad()
{
   using namespace name_literals;
   this->bind_vertex_shader("common/full_screen.vshader"_rc);
   this->set_vertex_topology(gapi::VertexTopology::TriangleFan);
   this->draw_primitives(4, 0);
}

void BuildContext::dispatch(const Vector3i dims)
{
   m_commands.emplace_back(std::in_place_type_t<detail::cmd::BindComputePipeline>{}, m_computePipelineState);
   m_computePipelineState.descriptorState.descriptorCount = 0;

   this->handle_descriptor_bindings();

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::Dispatch>{}, dims);

   ++m_descriptorCounts.totalDescriptorSets;
}

void BuildContext::fill_buffer_raw(const Name buffName, const void* ptr, const MemorySize size)
{
   m_activePipelineStage = gapi::PipelineStage::Transfer;

   this->prepare_buffer(buffName, gapi::BufferAccess::TransferWrite, gapi::BufferUsage::TransferDst);

   std::vector<u8> data(size);
   std::memcpy(data.data(), ptr, size);

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::FillBuffer>{}, buffName, std::move(data));

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::copy_texture_to_buffer(TextureRef srcTex, BufferRef dstBuff)
{
   m_activePipelineStage = gapi::PipelineStage::Transfer;

   if (std::holds_alternative<Name>(srcTex)) {
      this->prepare_texture(std::get<Name>(srcTex), gapi::TextureState::TransferSrc, gapi::TextureUsage::TransferSrc);
   }
   if (std::holds_alternative<Name>(dstBuff)) {
      this->prepare_buffer(std::get<Name>(dstBuff), gapi::BufferAccess::TransferWrite, gapi::BufferUsage::TransferDst);
   }

   this->add_command<detail::cmd::CopyTextureToBuffer>(srcTex, dstBuff);

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::copy_buffer(BufferRef srcBuffer, BufferRef dstBuffer)
{
   m_activePipelineStage = gapi::PipelineStage::Transfer;

   if (std::holds_alternative<Name>(srcBuffer)) {
      this->prepare_buffer(std::get<Name>(srcBuffer), gapi::BufferAccess::TransferRead, gapi::BufferUsage::TransferSrc);
   }
   if (std::holds_alternative<Name>(dstBuffer)) {
      this->prepare_buffer(std::get<Name>(dstBuffer), gapi::BufferAccess::TransferWrite, gapi::BufferUsage::TransferDst);
   }

   this->add_command<detail::cmd::CopyBuffer>(srcBuffer, dstBuffer);

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::declare_flag(const Name flagName)
{
   m_flags.emplace_back(flagName);
}

void BuildContext::if_enabled(const Name flag)
{
   this->add_command<detail::cmd::IfEnabledCond>(flag);
}

void BuildContext::if_disabled(const Name flag)
{
   this->add_command<detail::cmd::IfDisabledCond>(flag);
}

void BuildContext::end_if()
{
   this->add_command<detail::cmd::EndIfCond>();
}

Job BuildContext::build_job(PipelineCache& pipelineCache, ResourceStorage& storage)
{
   auto pool = this->create_descriptor_pool();

   std::vector<Job::Frame> frames;
   frames.reserve(FRAMES_IN_FLIGHT_COUNT);

   const auto flagCount = 1u << m_flags.size();

   this->create_resources(storage);

   for (const auto frameIndex : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
      DescriptorStorage descStorage;
      std::vector<gapi::CommandList> commandLists;

      for (const auto enabledFlags : Range(0u, flagCount)) {
         auto commandList = GAPI_CHECK(m_device.create_command_list(m_workTypes));
         GAPI_CHECK_STATUS(commandList.begin(gapi::SubmitType::Normal));

         this->write_commands(storage, descStorage, commandList, pipelineCache, pool.has_value() ? &(*pool) : nullptr, frameIndex,
                              enabledFlags);

         GAPI_CHECK_STATUS(commandList.finish());

         commandLists.emplace_back(std::move(commandList));
      }

      frames.emplace_back(std::move(descStorage), std::move(commandLists));
   }

   return {m_device, std::move(pool), frames, m_workTypes, m_flags};
}

void BuildContext::write_commands(ResourceStorage& storage, DescriptorStorage& descStorage, gapi::CommandList& cmdList,
                                  PipelineCache& cache, graphics_api::DescriptorPool* pool, const u32 frameIndex, const u32 enabledFlags)
{
   m_currentPipeline = nullptr;

   u32 requiredToBeEnabled = 0;
   u32 requiredToBeDisabled = 0;

   for (const auto& cmdVariant : m_commands) {
      std::visit(
         [&]<typename TCmd>(const TCmd& cmd) {
            if constexpr (std::is_same_v<TCmd, detail::cmd::IfEnabledCond>) {
               const auto flag = 1 << (std::ranges::find(m_flags, cmd.flag) - m_flags.begin());
               m_flagStack.emplace_front(flag, true);
               requiredToBeEnabled |= flag;
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::IfDisabledCond>) {
               const auto flag = 1 << (std::ranges::find(m_flags, cmd.flag) - m_flags.begin());
               m_flagStack.emplace_front(flag, false);
               requiredToBeDisabled |= flag;
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::EndIfCond>) {
               const auto [flag, isEnabled] = m_flagStack.front();
               m_flagStack.pop_front();
               if (isEnabled) {
                  requiredToBeEnabled &= ~flag;
               } else {
                  requiredToBeDisabled &= ~flag;
               }
            } else if ((requiredToBeEnabled == 0 || (enabledFlags & requiredToBeEnabled) != 0) &&
                       (requiredToBeDisabled == 0 || (enabledFlags & requiredToBeDisabled) == 0)) {
               if constexpr (std::is_same_v<TCmd, detail::cmd::BindComputePipeline>) {
                  auto* newPipeline = &cache.get_compute_pipeline(cmd.pso);
                  if (newPipeline != m_currentPipeline) {
                     m_currentPipeline = newPipeline;
                     cmdList.bind_pipeline(*m_currentPipeline);
                  }
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindGraphicsPipeline>) {
                  auto* newPipeline = &cache.get_graphics_pipeline(cmd.pso);
                  if (newPipeline != m_currentPipeline) {
                     m_currentPipeline = newPipeline;
                     cmdList.bind_pipeline(*m_currentPipeline);
                  }
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindDescriptors>) {
                  assert(cmd.descriptors.size() <= MAX_DESCRIPTOR_COUNT);
                  assert(pool != nullptr);
                  auto& descriptorArray = this->allocate_descriptors(descStorage, *pool);
                  this->write_descriptor(storage, descriptorArray[0], cmd, frameIndex);
                  cmdList.bind_descriptor_set(m_currentPipeline->pipeline_type(), descriptorArray[0]);
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::Dispatch>) {
                  cmdList.dispatch(cmd.dims.x, cmd.dims.y, cmd.dims.z);
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::CopyTextureToBuffer>) {
                  cmdList.copy_texture_to_buffer(this->resolve_texture_ref(storage, cmd.srcTexture, frameIndex),
                                                 this->resolve_buffer_ref(storage, cmd.dstBuffer, frameIndex));
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::CopyBuffer>) {
                  cmdList.copy_buffer(this->resolve_buffer_ref(storage, cmd.srcBuffer, frameIndex),
                                      this->resolve_buffer_ref(storage, cmd.dstBuffer, frameIndex));
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::PlaceBufferBarrier>) {
                  gapi::BufferBarrier barrier{};
                  barrier.buffer = &this->resolve_buffer_ref(storage, cmd.barrier->bufferRef, frameIndex);
                  barrier.srcAccess = cmd.barrier->srcBufferAccess;
                  barrier.dstAccess = cmd.barrier->dstBufferAccess;
                  cmdList.buffer_barrier(cmd.barrier->srcStageFlags, cmd.barrier->dstStageFlags, std::array{barrier});
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::PlaceTextureBarrier>) {
                  graphics_api::TextureBarrierInfo info{};
                  info.texture = &this->resolve_texture_ref(storage, cmd.barrier->textureRef, frameIndex);
                  info.sourceState = cmd.barrier->srcState;
                  info.targetState = cmd.barrier->dstState;
                  info.baseMipLevel = 0;
                  info.mipLevelCount = 1;
                  cmdList.texture_barrier(cmd.barrier->srcStageFlags, cmd.barrier->dstStageFlags, info);
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindVertexBuffer>) {
                  cmdList.bind_vertex_buffer(storage.buffer(cmd.buffName, frameIndex), 0);
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindIndexBuffer>) {
                  cmdList.bind_index_buffer(storage.buffer(cmd.buffName, frameIndex));
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::FillBuffer>) {
                  cmdList.update_buffer(storage.buffer(cmd.buffName, frameIndex), 0, cmd.data.size(), cmd.data.data());
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::BeginRenderPass>) {
                  auto renderingInfo = this->create_rendering_info(storage, cmd, frameIndex);
                  cmdList.begin_rendering(renderingInfo);
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::EndRenderPass>) {
                  cmdList.end_rendering();
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::DrawPrimitives>) {
                  cmdList.draw_primitives(cmd.vertexCount, cmd.vertexOffset, cmd.instanceCount, cmd.instanceOffset);
               } else if constexpr (std::is_same_v<TCmd, detail::cmd::DrawIndexedPrimitives>) {
                  cmdList.draw_indexed_primitives(cmd.indexCount, cmd.indexOffset, cmd.vertexOffset, cmd.instanceCount, cmd.instanceOffset);
               }
            }
         },
         cmdVariant);
   }
}

void BuildContext::create_resources(ResourceStorage& storage)
{
   for (const auto frameIndex : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
      for (const auto& declVariant : Values(m_declarations)) {
         std::visit(
            [this, frameIndex, &storage]<typename TDecl>(const TDecl& decl) {
               if constexpr (std::is_same_v<TDecl, detail::decl::Texture>) {
                  const auto size = decl.texDims.value_or(m_screenSize);
                  auto texture = GAPI_CHECK(
                     m_device.create_texture(decl.texFormat, {static_cast<u32>(size.x), static_cast<u32>(size.y)}, decl.texUsageFlags));
                  TG_SET_DEBUG_NAME(texture, resolve_name(decl.texName));
                  storage.register_texture(decl.texName, frameIndex, std::move(texture));
               } else if constexpr (std::is_same_v<TDecl, detail::decl::Buffer>) {
                  auto buffer = GAPI_CHECK(m_device.create_buffer(decl.buffUsageFlags, decl.buffSize));
                  TG_SET_DEBUG_NAME(buffer, resolve_name(decl.buffName));
                  storage.register_buffer(decl.buffName, frameIndex, std::move(buffer));
               }
            },
            declVariant);
      }
   }
}

void BuildContext::set_pipeline_state_descriptor(const graphics_api::PipelineStageFlags stages, const BindingIndex index,
                                                 const DescriptorInfo& info)
{
   assert(index < MAX_DESCRIPTOR_COUNT);
   if (stages & gapi::PipelineStage::FragmentShader || stages & gapi::PipelineStage::VertexShader) {
      m_graphicPipelineState.descriptorState.descriptors[index] = info;
      m_graphicPipelineState.descriptorState.descriptorCount = std::max(m_graphicPipelineState.descriptorState.descriptorCount, index + 1);
   }
   if (stages & gapi::PipelineStage::ComputeShader) {
      m_computePipelineState.descriptorState.descriptors[index] = info;
      m_computePipelineState.descriptorState.descriptorCount = std::max(m_computePipelineState.descriptorState.descriptorCount, index + 1);
   }
}

std::optional<gapi::DescriptorPool> BuildContext::create_descriptor_pool() const
{
   if (m_descriptorCounts.totalDescriptorSets == 0) {
      return std::nullopt;
   }

   const auto multiplier = FRAMES_IN_FLIGHT_COUNT * this->flag_variation_count();

   std::vector<std::pair<gapi::DescriptorType, u32>> descriptorCounts;
   if (m_descriptorCounts.storageTextureCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::StorageImage, multiplier * m_descriptorCounts.storageTextureCount);
   }
   if (m_descriptorCounts.uniformBufferCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::UniformBuffer, multiplier * m_descriptorCounts.uniformBufferCount);
   }
   if (m_descriptorCounts.samplableTextureCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::ImageSampler, multiplier * m_descriptorCounts.samplableTextureCount);
   }
   if (m_descriptorCounts.storageBufferCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::StorageBuffer, multiplier * m_descriptorCounts.storageBufferCount);
   }

   return GAPI_CHECK(m_device.create_descriptor_pool(descriptorCounts, multiplier * m_descriptorCounts.totalDescriptorSets));
}

graphics_api::WorkTypeFlags BuildContext::work_types() const
{
   return m_workTypes;
}

}// namespace triglav::render_core