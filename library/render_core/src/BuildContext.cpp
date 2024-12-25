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

namespace {

gapi::DescriptorType to_descriptor_type(const detail::Descriptor& desc)
{
   switch (desc.index()) {
   case 0:
      return gapi::DescriptorType::StorageImage;
   case 1:
      return gapi::DescriptorType::ImageSampler;
   default:
      assert(false);
      return gapi::DescriptorType{};
   }
}

}// namespace

BuildContext::BuildContext(graphics_api::Device& device, resource::ResourceManager& resourceManager) :
    m_device(device),
    m_resourceManager(resourceManager)
{
}

void BuildContext::declare_texture(Name texName, Vector2i texDims, gapi::ColorFormat texFormat)
{
   m_declarations.emplace(texName, detail::Declaration{std::in_place_type_t<detail::decl::Texture>{}, texName, texDims, texFormat});
}

void BuildContext::declare_render_target(Name rtName, gapi::ColorFormat rtFormat)
{
   gapi::ClearValue clearValue{.value = gapi::Color{0, 0, 0, 1}};
   m_declarations.emplace(rtName, detail::Declaration{std::in_place_type_t<detail::decl::RenderTarget>{}, rtName, std::nullopt, rtFormat,
                                                      clearValue, gapi::AttachmentAttribute::Color | gapi::AttachmentAttribute::ClearImage,
                                                      false, gapi::TextureUsage::ColorAttachment});
}

void BuildContext::declare_render_target_with_dims(Name rtName, Vector2i rtDims, gapi::ColorFormat rtFormat)
{
   gapi::ClearValue clearValue{.value = gapi::Color{0, 0, 0, 1}};
   m_declarations.emplace(rtName, detail::Declaration{std::in_place_type_t<detail::decl::RenderTarget>{}, rtName, rtDims, rtFormat,
                                                      clearValue, gapi::AttachmentAttribute::Color | gapi::AttachmentAttribute::ClearImage,
                                                      false, gapi::TextureUsage::ColorAttachment});
}

void BuildContext::declare_buffer(Name buffName, MemorySize size)
{
   m_declarations.emplace(buffName,
                          detail::Declaration{std::in_place_type_t<detail::decl::Buffer>{}, buffName, size, gapi::BufferUsage::None});
}

void BuildContext::declare_staging_buffer(Name buffName, MemorySize size)
{
   m_declarations.emplace(
      buffName, detail::Declaration{std::in_place_type_t<detail::decl::Buffer>{}, buffName, size, gapi::BufferUsage::HostVisible});
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

void BuildContext::bind_rw_texture(const BindingIndex index, const Name texName)
{
   this->setup_transition(texName, gapi::TextureState::General, m_activePipelineStage);

   ++m_descriptorCounts.storageImageCount;

   this->add_texture_flag(texName, gapi::TextureUsage::Storage);

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStage;
   descriptor.descriptorType = gapi::DescriptorType::StorageImage;
   this->write_descriptors(m_activePipelineStage, index, descriptor);

   this->set_descriptor<detail::descriptor::RWTexture>(index, texName);
}

void BuildContext::bind_uniform_buffer(const BindingIndex index, const Name buffName)
{
   this->setup_buffer_barrier(buffName, m_activePipelineStage);

   ++m_descriptorCounts.uniformBufferCount;

   this->add_buffer_flag(buffName, gapi::BufferUsage::UniformBuffer);

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStage;
   descriptor.descriptorType = gapi::DescriptorType::UniformBuffer;
   this->write_descriptors(m_activePipelineStage, index, descriptor);

   this->set_descriptor<detail::descriptor::UniformBuffer>(index, buffName);
}

void BuildContext::bind_vertex_layout(const VertexLayout& layout)
{
   m_graphicPipelineState.vertexLayout = layout;
}

void BuildContext::bind_vertex_buffer(const Name buffName)
{
   this->setup_buffer_barrier(buffName, gapi::PipelineStage::VertexInput);

   this->add_buffer_flag(buffName, gapi::BufferUsage::VertexBuffer);
   this->add_command<detail::cmd::BindVertexBuffer>(buffName);
}

void BuildContext::begin_render_pass_raw(const Name passName, const std::span<Name> renderTargets)
{
   for (const auto rtName : renderTargets) {
      const auto& renderTarget = this->declaration<detail::decl::RenderTarget>(rtName);

      const auto targetState = renderTarget.isDepthTarget ? gapi::TextureState::DepthTarget : gapi::TextureState::RenderTarget;
      this->setup_transition(rtName, targetState, gapi::PipelineStage::AttachmentOutput);

      if (renderTarget.isDepthTarget) {
         m_graphicPipelineState.depthTargetFormat = renderTarget.rtFormat;
      } else {
         m_graphicPipelineState.renderTargetFormats.emplace_back(renderTarget.rtFormat);
      }
   }

   std::vector<Name> renderTargetNames(renderTargets.size());
   std::ranges::copy(renderTargets, renderTargetNames.begin());
   this->add_command<detail::cmd::BeginRenderPass>(passName, std::move(renderTargetNames));
}

void BuildContext::end_render_pass()
{
   this->add_command<detail::cmd::EndRenderPass>();
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
                                    const detail::cmd::BindDescriptors& descriptors) const
{
   graphics_api::DescriptorWriter writer(m_device, descView);

   for (const auto [index, descVariant] : Enumerate(descriptors.descriptors)) {
      std::visit(
         [this, index, &writer, &storage]<typename TDescriptor>(const TDescriptor& desc) {
            if constexpr (std::is_same_v<TDescriptor, detail::descriptor::RWTexture>) {
               writer.set_storage_image(index, storage.texture(desc.texName));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::CombinedTexSampler>) {
               writer.set_texture_only(index, m_resourceManager.get(desc.texName));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBuffer>) {
               writer.set_raw_uniform_buffer(index, storage.buffer(desc.buffName));
            }
         },
         *descVariant);
   }
}

void BuildContext::add_texture_flag(const Name texName, const graphics_api::TextureUsage flag)
{
   if (this->is_render_target(texName)) {
      this->declaration<detail::decl::RenderTarget>(texName).texUsageFlags |= flag;
   } else {
      this->declaration<detail::decl::Texture>(texName).texUsageFlags |= flag;
   }
}

void BuildContext::add_buffer_flag(const Name buffName, const graphics_api::BufferUsage flag)
{
   std::get<detail::decl::Buffer>(m_declarations[buffName]).buffUsageFlags |= flag;
}

void BuildContext::set_buffer_in_transfer_state(const Name buffName, const bool value, graphics_api::PipelineStage pipelineStage)
{
   auto& buff = std::get<detail::decl::Buffer>(m_declarations[buffName]);
   buff.lastUsedStage = pipelineStage;
   buff.isInTransferState = value;
}

bool BuildContext::is_buffer_in_transfer_state(const Name buffName) const
{
   return std::get<detail::decl::Buffer>(m_declarations.at(buffName)).isInTransferState;
}

void BuildContext::setup_transition(const Name texName, const graphics_api::TextureState targetState,
                                    graphics_api::PipelineStage targetStage)
{
   auto [lastUsedStage, currentState] = [this, texName] {
      if (this->is_render_target(texName)) {
         auto& rt = this->declaration<detail::decl::RenderTarget>(texName);
         return std::pair<graphics_api::PipelineStage&, graphics_api::TextureState&>{rt.lastUsedStage, rt.currentState};
      } else {
         auto& tex = this->declaration<detail::decl::Texture>(texName);
         return std::pair<graphics_api::PipelineStage&, graphics_api::TextureState&>{tex.lastUsedStage, tex.currentState};
      }
   }();

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::TextureTransition>{}, texName, lastUsedStage, currentState, targetStage,
                           targetState);

   lastUsedStage = targetStage;
   currentState = targetState;
}

void BuildContext::setup_buffer_barrier(const Name buffName, const graphics_api::PipelineStage targetStage)
{
   auto& buffer = std::get<detail::decl::Buffer>(m_declarations[buffName]);
   if (!buffer.isInTransferState) {
      return;
   }

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::ExecutionBarrier>{}, buffer.lastUsedStage, targetStage);

   buffer.isInTransferState = false;
   buffer.lastUsedStage = targetStage;
}

gapi::RenderingInfo BuildContext::create_rendering_info(ResourceStorage& storage, const detail::cmd::BeginRenderPass& beginRenderPass) const
{
   gapi::RenderingInfo info{};

   gapi::Resolution resolution{};

   for (const auto rtName : beginRenderPass.renderTargets) {
      const auto& decl = this->declaration<detail::decl::RenderTarget>(rtName);

      gapi::RenderAttachment attachment{};
      attachment.texture = &storage.texture(rtName);
      attachment.state = decl.isDepthTarget ? gapi::TextureState::DepthTarget : gapi::TextureState::RenderTarget;
      attachment.clearValue = decl.clearValue;
      attachment.flags = decl.flags;

      resolution = attachment.texture->resolution();

      if (decl.isDepthTarget) {
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

bool BuildContext::is_depth_target(const Name rtName) const
{
   return this->declaration<detail::decl::RenderTarget>(rtName).isDepthTarget;
}

bool BuildContext::is_render_target(const Name texName) const
{
   return std::holds_alternative<detail::decl::RenderTarget>(this->m_declarations.at(texName));
}

void BuildContext::draw_primitives(const u32 vertexCount, const u32 vertexOffset)
{
   m_commands.emplace_back(std::in_place_type_t<detail::cmd::BindGraphicsPipeline>{}, m_graphicPipelineState);
   m_graphicPipelineState.descriptorState.descriptorCount = 0;

   this->handle_descriptor_bindings();

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::DrawPrimitives>{}, vertexCount, vertexOffset, 1, 0);

   ++m_descriptorCounts.totalDescriptorSets;
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
   this->add_buffer_flag(buffName, gapi::BufferUsage::TransferDst);
   this->set_buffer_in_transfer_state(buffName, true, gapi::PipelineStage::Transfer);

   std::vector<u8> data(size);
   std::memcpy(data.data(), ptr, size);

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::FillBuffer>{}, buffName, std::move(data));

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::copy_texture_to_buffer(Name srcTex, Name dstBuff)
{
   this->add_texture_flag(srcTex, gapi::TextureUsage::TransferSrc);
   this->add_buffer_flag(dstBuff, gapi::BufferUsage::TransferDst);
   this->set_buffer_in_transfer_state(dstBuff, true, gapi::PipelineStage::Transfer);

   this->setup_transition(srcTex, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);

   this->add_command<detail::cmd::CopyTextureToBuffer>(srcTex, dstBuff);

   m_workTypes |= gapi::WorkType::Transfer;
}

Job BuildContext::build_job(PipelineCache& pipelineCache, const std::span<ResourceStorage> storages)
{
   assert(storages.size() >= FRAMES_IN_FLIGHT_COUNT);

   auto pool = this->create_descriptor_pool();

   std::vector<Job::Frame> frames;
   frames.reserve(FRAMES_IN_FLIGHT_COUNT);

   for (const auto frameIndex : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
      this->create_resources(storages[frameIndex]);

      auto commandList = GAPI_CHECK(m_device.create_command_list(m_workTypes));
      GAPI_CHECK_STATUS(commandList.begin(gapi::SubmitType::Normal));

      DescriptorStorage descStorage;

      this->write_commands(storages[frameIndex], descStorage, commandList, pipelineCache, pool);

      GAPI_CHECK_STATUS(commandList.finish());

      frames.emplace_back(std::move(descStorage), std::move(commandList));
   }

   return Job(m_device, std::move(pool), frames, m_workTypes);
}

void BuildContext::write_commands(ResourceStorage& storage, DescriptorStorage& descStorage, gapi::CommandList& cmdList,
                                  PipelineCache& cache, graphics_api::DescriptorPool& pool)
{
   m_currentPipeline = nullptr;

   for (const auto& cmdVariant : m_commands) {
      std::visit(
         [&]<typename TCmd>(const TCmd& cmd) {
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
               auto& descriptorArray = this->allocate_descriptors(descStorage, pool);
               this->write_descriptor(storage, descriptorArray[0], cmd);
               cmdList.bind_descriptor_set(m_currentPipeline->pipeline_type(), descriptorArray[0]);
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::Dispatch>) {
               cmdList.dispatch(cmd.dims.x, cmd.dims.y, cmd.dims.z);
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::CopyTextureToBuffer>) {
               cmdList.copy_texture_to_buffer(storage.texture(cmd.srcTexture), storage.buffer(cmd.dstBuffer));
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::TextureTransition>) {
               graphics_api::TextureBarrierInfo info{};
               info.texture = &storage.texture(cmd.texName);
               info.sourceState = cmd.srcState;
               info.targetState = cmd.dstState;
               info.baseMipLevel = 0;
               info.mipLevelCount = 1;
               cmdList.texture_barrier(cmd.srcStageFlags, cmd.dstStageFlags, info);
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindVertexBuffer>) {
               cmdList.bind_vertex_buffer(storage.buffer(cmd.buffName), 0);
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::FillBuffer>) {
               cmdList.update_buffer(storage.buffer(cmd.buffName), 0, cmd.data.size(), cmd.data.data());
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::BeginRenderPass>) {
               auto renderingInfo = this->create_rendering_info(storage, cmd);
               cmdList.begin_rendering(renderingInfo);
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::EndRenderPass>) {
               cmdList.end_rendering();
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::DrawPrimitives>) {
               cmdList.draw_primitives(cmd.vertexCount, cmd.vertexOffset, cmd.instanceCount, cmd.instanceOffset);
            }
         },
         cmdVariant);
   }
}

void BuildContext::create_resources(ResourceStorage& storage)
{
   for (const auto& declVariant : Values(m_declarations)) {
      std::visit(
         [this, &storage]<typename TDecl>(const TDecl& decl) {
            if constexpr (std::is_same_v<TDecl, detail::decl::Texture>) {
               auto texture = GAPI_CHECK(m_device.create_texture(decl.texFormat, {decl.texDims.x, decl.texDims.y}, decl.texUsageFlags));
               TG_SET_DEBUG_NAME(texture, resolve_name(decl.texName));
               storage.register_texture(decl.texName, std::move(texture));
            } else if constexpr (std::is_same_v<TDecl, detail::decl::RenderTarget>) {
               assert(decl.rtDims.has_value());
               auto texture = GAPI_CHECK(m_device.create_texture(decl.rtFormat, {decl.rtDims->x, decl.rtDims->y}, decl.texUsageFlags));
               TG_SET_DEBUG_NAME(texture, resolve_name(decl.rtName));
               storage.register_texture(decl.rtName, std::move(texture));
            } else if constexpr (std::is_same_v<TDecl, detail::decl::Buffer>) {
               auto buffer = GAPI_CHECK(m_device.create_buffer(decl.buffUsageFlags, decl.buffSize));
               TG_SET_DEBUG_NAME(buffer, resolve_name(decl.buffName));
               storage.register_buffer(decl.buffName, std::move(buffer));
            }
         },
         declVariant);
   }
}

void BuildContext::write_descriptors(const graphics_api::PipelineStageFlags stages, const BindingIndex index, const DescriptorInfo& info)
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

gapi::DescriptorPool BuildContext::create_descriptor_pool() const
{
   std::vector<std::pair<gapi::DescriptorType, u32>> descriptorCounts;
   if (m_descriptorCounts.storageImageCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::StorageImage, FRAMES_IN_FLIGHT_COUNT * m_descriptorCounts.storageImageCount);
   }
   if (m_descriptorCounts.uniformBufferCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::UniformBuffer, FRAMES_IN_FLIGHT_COUNT * m_descriptorCounts.uniformBufferCount);
   }

   return GAPI_CHECK(m_device.create_descriptor_pool(descriptorCounts, FRAMES_IN_FLIGHT_COUNT * m_descriptorCounts.totalDescriptorSets));
}

graphics_api::WorkTypeFlags BuildContext::work_types() const
{
   return m_workTypes;
}

void BuildContext::write_commands()
{
   for (const auto& cmdVariant : m_commands) {
      std::visit(
         []<typename TCmd>(const TCmd& cmd) {
            if constexpr (std::is_same_v<TCmd, detail::cmd::BindGraphicsPipeline>) {
               fmt::print("BindGraphicsPSO(Hash: {})\n", cmd.pso.hash());
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindComputePipeline>) {
               fmt::print("BindComputePSO(Hash: {})\n", cmd.pso.hash());
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindDescriptors>) {
               fmt::print("BindDescriptors(...)\n");
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::BindVertexBuffer>) {
               fmt::print("BindVertexBuffer({})\n", resolve_name(cmd.buffName));
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::TextureTransition>) {
               fmt::print("TextureTransition({})\n", resolve_name(cmd.texName));
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::ExecutionBarrier>) {
               fmt::print("ExecutionBarrier(...)\n");
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::DrawPrimitives>) {
               fmt::print("DrawPrimitives(VertexCount: {}, VertexOffset: {}, InstanceCount: {}, InstanceOffset: {})\n", cmd.vertexCount,
                          cmd.vertexOffset, cmd.instanceCount, cmd.instanceOffset);
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::Dispatch>) {
               fmt::print("Dispatch(X: {}, Y: {}, Z: {})\n", cmd.dims.x, cmd.dims.y, cmd.dims.z);
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::CopyTextureToBuffer>) {
               fmt::print("CopyTextureToBuffer(Src: {}, Dst: {})\n", resolve_name(cmd.srcTexture), resolve_name(cmd.dstBuffer));
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::FillBuffer>) {
               fmt::print("FillBuffer({}, ...)\n", resolve_name(cmd.buffName));
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::BeginRenderPass>) {
               fmt::print("BeginRenderPass(Name: {}, Targets: [", resolve_name(cmd.passName));
               bool isFirst = true;
               for (const auto& target : cmd.renderTargets) {
                  if (isFirst) {
                     isFirst = false;
                  } else {
                     fmt::print(", ");
                  }
                  fmt::print("{}", resolve_name(target));
               }
               fmt::print("])\n");
            } else if constexpr (std::is_same_v<TCmd, detail::cmd::EndRenderPass>) {
               fmt::print("EndRenderPass(...)\n");
            }
         },
         cmdVariant);
   }
}

}// namespace triglav::render_core