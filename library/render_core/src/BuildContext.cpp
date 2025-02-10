#include "BuildContext.hpp"

#include "ApplyFlagConditionsPass.hpp"
#include "BarrierInsertionPass.hpp"
#include "GenerateCommandListPass.hpp"

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

[[maybe_unused]] std::string create_command_list_name(const Name jobName, const u32 frameIndex, const u32 enabledFlags)
{
   if (jobName == 0)
      return {};

   const auto resolvedName = resolve_name(jobName);
   if (resolvedName.empty())
      return {};

   return std::format("{}.frame{}.flags{}", resolvedName, frameIndex, enabledFlags);
}

[[maybe_unused]] std::string create_object_name(const Name jobName, const u32 frameIndex)
{
   if (jobName == 0)
      return {};

   const auto resolvedName = resolve_name(jobName);
   if (resolvedName.empty())
      return {};

   return std::format("{}.frame{}", resolvedName, frameIndex);
}

}// namespace

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

void BuildContext::declare_screen_size_texture(const Name texName, const gapi::ColorFormat texFormat)
{
   this->add_declaration<detail::decl::Texture>(texName, std::nullopt, texFormat);
}

void BuildContext::declare_proportional_texture(const Name texName, const graphics_api::ColorFormat texFormat, const float scale,
                                                const bool createMipLevels)
{
   this->add_declaration<detail::decl::Texture>(texName, std::nullopt, texFormat, gapi::TextureUsage::None, createMipLevels, scale);
}

void BuildContext::declare_render_target(const Name rtName, gapi::ColorFormat rtFormat)
{
   this->m_renderTargets.emplace(rtName, detail::RenderTarget{gapi::ClearValue::color(gapi::ColorPalette::Black),
                                                              gapi::AttachmentAttribute::Color | gapi::AttachmentAttribute::ClearImage |
                                                                 gapi::AttachmentAttribute::StoreImage});
   this->add_declaration<detail::decl::Texture>(rtName, std::nullopt, rtFormat, gapi::TextureUsage::ColorAttachment);
}

void BuildContext::declare_depth_target(Name dtName, graphics_api::ColorFormat rtFormat)
{
   this->m_renderTargets.emplace(dtName, detail::RenderTarget{gapi::ClearValue::depth_stencil(1.0f, 0),
                                                              gapi::AttachmentAttribute::Depth | gapi::AttachmentAttribute::ClearImage});
   this->add_declaration<detail::decl::Texture>(dtName, std::nullopt, rtFormat, gapi::TextureUsage::DepthStencilAttachment);
}

void BuildContext::declare_proportional_depth_target(Name dtName, graphics_api::ColorFormat rtFormat, float scale)
{
   this->m_renderTargets.emplace(dtName, detail::RenderTarget{gapi::ClearValue::depth_stencil(1.0f, 0),
                                                              gapi::AttachmentAttribute::Depth | gapi::AttachmentAttribute::ClearImage});
   this->add_declaration<detail::decl::Texture>(dtName, std::nullopt, rtFormat, gapi::TextureUsage::DepthStencilAttachment, false, scale);
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

void BuildContext::declare_proportional_buffer(const Name buffName, const float scale, const MemorySize stride)
{
   this->add_declaration<detail::decl::Buffer>(buffName, stride, gapi::BufferUsage::None, scale);
}

void BuildContext::bind_vertex_shader(VertexShaderName vsName)
{
   m_workTypes |= gapi::WorkType::Graphics;
   m_graphicPipelineState.vertexShader = vsName;
   m_activePipelineStages = gapi::PipelineStage::VertexShader;
}

void BuildContext::bind_fragment_shader(FragmentShaderName fsName)
{
   m_workTypes |= gapi::WorkType::Graphics;
   m_graphicPipelineState.fragmentShader = fsName;
   m_activePipelineStages = gapi::PipelineStage::FragmentShader;
}

void BuildContext::bind_compute_shader(ComputeShaderName csName)
{
   m_workTypes |= gapi::WorkType::Compute;
   m_computePipelineState.computeShader = csName;
   m_activePipelineStages = gapi::PipelineStage::ComputeShader;
}

void BuildContext::bind_rw_texture(const BindingIndex index, const TextureRef texRef)
{
   this->prepare_texture(texRef, gapi::TextureState::General, gapi::TextureUsage::Storage);

   ++m_descriptorCounts.storageTextureCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::StorageImage;
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   this->set_descriptor<detail::descriptor::RWTexture>(index, m_activePipelineStages, texRef);
}

void BuildContext::bind_samplable_texture(const BindingIndex index, const TextureRef texRef)
{
   this->prepare_texture(texRef, gapi::TextureState::ShaderRead, gapi::TextureUsage::Sampled);

   ++m_descriptorCounts.sampledTextureCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::ImageSampler;
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   this->set_descriptor<detail::descriptor::SamplableTexture>(index, m_activePipelineStages, texRef);
}

void BuildContext::bind_texture(const BindingIndex index, const TextureRef texRef)
{
   this->prepare_texture(texRef, gapi::TextureState::ShaderRead, gapi::TextureUsage::Sampled);

   ++m_descriptorCounts.textureCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::ImageOnly;
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   this->set_descriptor<detail::descriptor::Texture>(index, m_activePipelineStages, texRef);
}

void BuildContext::bind_sampled_texture_array(const BindingIndex index, std::span<const TextureRef> texRefs)
{
   for (const auto& ref : texRefs) {
      this->prepare_texture(ref, gapi::TextureState::ShaderRead, gapi::TextureUsage::Sampled);
   }

   m_descriptorCounts.sampledTextureCount += texRefs.size();

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::ImageSampler;
   descriptor.descriptorCount = texRefs.size();
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   std::vector<TextureRef> textureRefs;
   textureRefs.reserve(texRefs.size());
   std::ranges::copy(texRefs, std::back_inserter(textureRefs));
   this->set_descriptor<detail::descriptor::SampledTextureArray>(index, m_activePipelineStages, std::move(textureRefs));
}

void BuildContext::bind_uniform_buffer(const BindingIndex index, const BufferRef buffRef)
{
   this->prepare_buffer(buffRef, gapi::BufferUsage::UniformBuffer);

   ++m_descriptorCounts.uniformBufferCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::UniformBuffer;
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   this->set_descriptor<detail::descriptor::UniformBuffer>(index, m_activePipelineStages, buffRef);
}

void BuildContext::bind_uniform_buffers(const BindingIndex index, const std::span<const BufferRef> buffers)
{
   for (const auto& buffer : buffers) {
      this->prepare_buffer(buffer, gapi::BufferUsage::UniformBuffer);
   }

   m_descriptorCounts.uniformBufferCount += buffers.size();

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::UniformBuffer;
   descriptor.descriptorCount = buffers.size();
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   std::vector<BufferRef> bufferRefs;
   bufferRefs.reserve(buffers.size());
   std::ranges::copy(buffers, std::back_inserter(bufferRefs));
   this->set_descriptor<detail::descriptor::UniformBufferArray>(index, m_activePipelineStages, std::move(bufferRefs));
}

void BuildContext::bind_storage_buffer(const BindingIndex index, const BufferRef buffRef)
{
   this->prepare_buffer(buffRef, gapi::BufferUsage::StorageBuffer);

   ++m_descriptorCounts.storageBufferCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::StorageBuffer;
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   this->set_descriptor<detail::descriptor::StorageBuffer>(index, m_activePipelineStages, buffRef);
}

void BuildContext::push_constant_span(const std::span<const u8> buffer)
{
   if (m_activePipelineStages == gapi::PipelineStage::FragmentShader || m_activePipelineStages == gapi::PipelineStage::VertexShader) {
      m_graphicPipelineState.pushConstants.emplace_back(m_activePipelineStages, buffer.size());
   } else if (m_activePipelineStages & gapi::PipelineStage::RayGenerationShader ||
              m_activePipelineStages & gapi::PipelineStage::MissShader || m_activePipelineStages & gapi::PipelineStage::ClosestHitShader) {
      m_rayTracingPipelineState.pushConstants.emplace_back(m_activePipelineStages, buffer.size());
   }

   std::vector data(buffer.begin(), buffer.end());
   m_pendingPushConstants.emplace_back(m_activePipelineStages, std::move(data));
}

void BuildContext::bind_vertex_layout(const VertexLayout& layout)
{
   m_graphicPipelineState.vertexLayout = layout;
}

void BuildContext::bind_vertex_buffer(const BufferRef buffRef)
{
   m_activePipelineStages = gapi::PipelineStage::VertexInput;

   this->prepare_buffer(buffRef, gapi::BufferUsage::VertexBuffer);

   this->add_command<detail::cmd::BindVertexBuffer>(buffRef);
}

void BuildContext::bind_index_buffer(const BufferRef buffRef)
{
   m_activePipelineStages = gapi::PipelineStage::VertexInput;

   this->prepare_buffer(buffRef, gapi::BufferUsage::IndexBuffer);

   this->add_command<detail::cmd::BindIndexBuffer>(buffRef);
}

void BuildContext::begin_render_pass_raw(const Name passName, const std::span<Name> renderTargets)
{
   for (const auto rtName : renderTargets) {
      const auto& rtTexture = this->declaration<detail::decl::Texture>(rtName);
      const auto& renderTarget = m_renderTargets.at(rtName);

      if (renderTarget.flags & gapi::AttachmentAttribute::Depth) {
         m_graphicPipelineState.depthTargetFormat = rtTexture.texFormat;
      } else {
         m_graphicPipelineState.renderTargetFormats.emplace_back(rtTexture.texFormat);
      }
   }

   std::vector<Name> renderTargetNames(renderTargets.size());
   std::ranges::copy(renderTargets, renderTargetNames.begin());
   this->add_command<detail::cmd::BeginRenderPass>(passName, std::move(renderTargetNames));
}

void BuildContext::end_render_pass()
{
   m_graphicPipelineState.depthTargetFormat.reset();
   m_graphicPipelineState.renderTargetFormats.clear();
   this->add_command<detail::cmd::EndRenderPass>();
}

void BuildContext::clear_color(const Name targetName, const Vector4 color)
{
   m_renderTargets.at(targetName).clearValue.value.emplace<gapi::Color>(color.x, color.y, color.z, color.w);
}

void BuildContext::reset_queries(const u32 offset, const u32 count)
{
   this->add_command<detail::cmd::ResetQueries>(offset, count);
}

void BuildContext::query_timestamp(const u32 index, const bool isClosing)
{
   this->add_command<detail::cmd::QueryTimestamp>(index, isClosing);
}

void BuildContext::handle_descriptor_bindings()
{
   if (m_descriptors.empty()) {
      return;
   }

   for ([[maybe_unused]] const auto& descriptor : m_descriptors) {
      assert(descriptor.has_value());
   }

   this->add_command<detail::cmd::BindDescriptors>(std::move(m_descriptors));
}

graphics_api::DescriptorArray& BuildContext::allocate_descriptors(DescriptorStorage& storage, graphics_api::DescriptorPool& pool,
                                                                  graphics_api::Pipeline& pipeline) const
{
   graphics_api::DescriptorLayoutArray descriptorLayouts;
   descriptorLayouts.add_from_pipeline(pipeline);

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
               writer.set_storage_image_view(index, this->resolve_texture_view_ref(storage, desc.texRef, frameIndex));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SamplableTexture>) {
               const gapi::Texture& texture = this->resolve_texture_ref(storage, desc.texRef, frameIndex);
               writer.set_sampled_texture(index, texture, m_device.sampler_cache().find_sampler(texture.sampler_properties()));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SampledTextureArray>) {
               std::vector<const gapi::Texture*> textures;
               textures.reserve(desc.texRefs.size());
               for (const auto& texRef : desc.texRefs) {
                  textures.push_back(&this->resolve_texture_ref(storage, texRef, frameIndex));
               }
               writer.set_texture_array(index, textures);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::Texture>) {
               writer.set_texture_view_only(index, this->resolve_texture_view_ref(storage, desc.texRef, frameIndex));
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
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::AccelerationStructure>) {
               writer.set_acceleration_structure(index, *desc.accelerationStructure);
            }
         },
         descVariant->descriptor);
   }
}

void BuildContext::add_texture_usage(const Name texName, const graphics_api::TextureUsageFlags flags)
{
   this->declaration<detail::decl::Texture>(texName).texUsageFlags |= flags;
}

void BuildContext::add_buffer_usage(const Name buffName, const graphics_api::BufferUsageFlags flags)
{
   this->declaration<detail::decl::Buffer>(buffName).buffUsageFlags |= flags;
}

void BuildContext::set_bind_stages(graphics_api::PipelineStageFlags stages)
{
   m_activePipelineStages = stages;
}

bool BuildContext::is_ray_tracing_supported() const
{
   return m_device.enabled_features() & gapi::DeviceFeature::RayTracing;
}

void BuildContext::export_texture(const Name texName, const graphics_api::PipelineStage pipelineStage,
                                  const graphics_api::TextureState state, const graphics_api::TextureUsageFlags flags)
{
   m_activePipelineStages = pipelineStage;
   this->prepare_texture(texName, state, flags);
   this->add_command<detail::cmd::ExportTexture>(texName, pipelineStage, state);
}

void BuildContext::export_buffer(const Name buffName, const graphics_api::PipelineStage pipelineStage,
                                 const graphics_api::BufferAccess access, const graphics_api::BufferUsageFlags flags)
{
   this->add_buffer_usage(buffName, flags);
   this->add_command<detail::cmd::ExportBuffer>(buffName, pipelineStage, access);
}

graphics_api::SamplerProperties& BuildContext::texture_sampler_properties(const Name name)
{
   return this->declaration<detail::decl::Texture>(name).samplerProperties;
}

void BuildContext::set_sampler_properties(const Name name, const graphics_api::SamplerProperties& samplerProps)
{
   this->texture_sampler_properties(name) = samplerProps;
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

const gapi::Texture& BuildContext::resolve_texture_ref(ResourceStorage& storage, TextureRef texRef, u32 frameIndex) const
{
   return std::visit(
      [this, frameIndex, &storage]<typename TVariant>(const TVariant& var) -> const gapi::Texture& {
         if constexpr (std::is_same_v<TVariant, Name>) {
            return storage.texture(var, frameIndex);
         } else if constexpr (std::is_same_v<TVariant, External>) {
            return storage.texture(var.name, frameIndex);
         } else if constexpr (std::is_same_v<TVariant, TextureMip>) {
            return storage.texture(var.name, frameIndex);
         } else if constexpr (std::is_same_v<TVariant, FromLastFrame>) {
            return storage.texture(var.name, (FRAMES_IN_FLIGHT_COUNT + frameIndex - 1) % FRAMES_IN_FLIGHT_COUNT);
         } else if constexpr (std::is_same_v<TVariant, TextureName>) {
            return m_resourceManager.get(var);
         } else if constexpr (std::is_same_v<TVariant, const graphics_api::Texture*>) {
            return *var;
         } else {
            assert(0);
         }
      },
      texRef);
}

const graphics_api::TextureView& BuildContext::resolve_texture_view_ref(ResourceStorage& storage, const TextureRef texRef,
                                                                        const u32 frameIndex) const
{
   if (std::holds_alternative<TextureMip>(texRef)) {
      const auto& textureMip = std::get<TextureMip>(texRef);
      return storage.texture_mip_view(textureMip.name, textureMip.mipLevel, frameIndex);
   }

   return this->resolve_texture_ref(storage, texRef, frameIndex).view();
}

const graphics_api::Buffer& BuildContext::resolve_buffer_ref(ResourceStorage& storage, BufferRef buffRef, u32 frameIndex) const
{
   return std::visit(
      [frameIndex, &storage]<typename TVariant>(const TVariant& var) -> const gapi::Buffer& {
         if constexpr (std::is_same_v<TVariant, Name>) {
            return storage.buffer(var, frameIndex);
         } else if constexpr (std::is_same_v<TVariant, External>) {
            return storage.buffer(var.name, frameIndex);
         } else if constexpr (std::is_same_v<TVariant, FromLastFrame>) {
            return storage.buffer(var.name, (FRAMES_IN_FLIGHT_COUNT + frameIndex - 1) % FRAMES_IN_FLIGHT_COUNT);
         } else if constexpr (std::is_same_v<TVariant, const gapi::Buffer*>) {
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

   for (auto&& pushConstant : m_pendingPushConstants) {
      this->add_command<detail::cmd::PushConstant>(std::move(pushConstant));
   }
   m_pendingPushConstants.clear();

   m_graphicPipelineState.descriptorState.descriptorCount = 0;
   m_graphicPipelineState.vertexLayout.stride = 0;
   m_graphicPipelineState.vertexLayout.attributes.clear();
   m_graphicPipelineState.vertexTopology = graphics_api::VertexTopology::TriangleList;
   m_graphicPipelineState.depthTestMode = graphics_api::DepthTestMode::Enabled;
   m_graphicPipelineState.pushConstants.clear();
   m_graphicPipelineState.isBlendingEnabled = true;

   ++m_descriptorCounts.totalDescriptorSets;

   this->handle_descriptor_bindings();
}

void BuildContext::prepare_texture(const TextureRef texRef, const gapi::TextureState state, const gapi::TextureUsageFlags usage)
{
   if (!std::holds_alternative<Name>(texRef) && !std::holds_alternative<FromLastFrame>(texRef) &&
       !std::holds_alternative<TextureMip>(texRef)) {
      return;
   }

   const Name texName = std::holds_alternative<Name>(texRef)            ? std::get<Name>(texRef)
                        : std::holds_alternative<FromLastFrame>(texRef) ? std::get<FromLastFrame>(texRef).name
                                                                        : std::get<TextureMip>(texRef).name;

   this->add_texture_usage(texName, usage);

   if (gapi::to_memory_access(state) == gapi::MemoryAccess::Read && m_renderTargets.contains(texName)) {
      auto& renderTarget = m_renderTargets.at(texName);
      renderTarget.flags |= gapi::AttachmentAttribute::StoreImage;
   }
   if (gapi::to_memory_access(state) == gapi::MemoryAccess::Write && m_renderTargets.contains(texName)) {
      auto& renderTarget = m_renderTargets.at(texName);
      renderTarget.flags |= gapi::AttachmentAttribute::LoadImage;
      renderTarget.flags.remove_flag(gapi::AttachmentAttribute::ClearImage);
   }
}

void BuildContext::prepare_buffer(const BufferRef buffRef, const gapi::BufferUsage usage)
{
   if (!std::holds_alternative<Name>(buffRef) && !std::holds_alternative<FromLastFrame>(buffRef)) {
      return;
   }

   const Name buffName = std::holds_alternative<Name>(buffRef) ? std::get<Name>(buffRef) : std::get<FromLastFrame>(buffRef).name;

   this->add_buffer_usage(buffName, usage);
}

u32 BuildContext::flag_variation_count() const
{
   return 1u << m_flags.size();
}

void BuildContext::reset_resource_states()
{
   for (auto& decl : Values(m_declarations)) {
      std::visit(
         []<typename TDecl>(TDecl& decl) {
            if constexpr (std::is_same_v<TDecl, detail::decl::Texture>) {
               decl.currentStatePerMip.fill(graphics_api::TextureState::Undefined);
               decl.lastStages.fill(graphics_api::PipelineStage::Entrypoint);
               decl.lastTextureBarrier = nullptr;
            } else if constexpr (std::is_same_v<TDecl, detail::decl::Buffer>) {
               decl.currentAccess = gapi::BufferAccess::None;
               decl.lastStages = graphics_api::PipelineStage::Entrypoint;
               decl.lastBufferBarrier = nullptr;
            }
         },
         decl);
   }
}

void BuildContext::set_vertex_topology(const graphics_api::VertexTopology topology)
{
   m_graphicPipelineState.vertexTopology = topology;
}

void BuildContext::set_depth_test_mode(const graphics_api::DepthTestMode mode)
{
   m_graphicPipelineState.depthTestMode = mode;
}

void BuildContext::set_is_blending_enabled(const bool enabled)
{
   m_graphicPipelineState.isBlendingEnabled = enabled;
}

void BuildContext::draw_primitives(const u32 vertexCount, const u32 vertexOffset, u32 instanceCount, u32 instanceOffset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawPrimitives>(vertexCount, vertexOffset, instanceCount, instanceOffset);
}

void BuildContext::draw_indexed_primitives(const u32 indexCount, const u32 indexOffset, const u32 vertexOffset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedPrimitives>(indexCount, indexOffset, vertexOffset, 1u, 0u);
}

void BuildContext::draw_indexed_primitives(const u32 indexCount, const u32 indexOffset, const u32 vertexOffset, const u32 instanceCount,
                                           const u32 instanceOffset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedPrimitives>(indexCount, indexOffset, vertexOffset, instanceCount, instanceOffset);
}

void BuildContext::draw_indexed_indirect_with_count(const BufferRef drawCallBuffer, const BufferRef countBuffer, const u32 maxDrawCalls,
                                                    const u32 stride)
{
   this->prepare_buffer(drawCallBuffer, gapi::BufferUsage::Indirect);
   this->prepare_buffer(countBuffer, gapi::BufferUsage::Indirect);

   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedIndirectWithCount>(drawCallBuffer, countBuffer, maxDrawCalls, stride);
}

void BuildContext::draw_indirect_with_count(BufferRef drawCallBuffer, BufferRef countBuffer, u32 maxDrawCalls, u32 stride)
{
   this->prepare_buffer(drawCallBuffer, gapi::BufferUsage::Indirect);
   this->prepare_buffer(countBuffer, gapi::BufferUsage::Indirect);

   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndirectWithCount>(drawCallBuffer, countBuffer, maxDrawCalls, stride);
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
   assert(dims.x > 0 && dims.y > 0 && dims.z > 0);

   this->add_command<detail::cmd::BindComputePipeline>(m_computePipelineState);
   m_computePipelineState.descriptorState.descriptorCount = 0;

   this->handle_descriptor_bindings();

   this->add_command<detail::cmd::Dispatch>(dims);

   ++m_descriptorCounts.totalDescriptorSets;
}

void BuildContext::dispatch_indirect(const BufferRef indirectBuffer)
{
   this->prepare_buffer(indirectBuffer, gapi::BufferUsage::Indirect);

   this->add_command<detail::cmd::BindComputePipeline>(m_computePipelineState);
   m_computePipelineState.descriptorState.descriptorCount = 0;

   this->handle_descriptor_bindings();

   this->add_command<detail::cmd::DispatchIndirect>(indirectBuffer);

   ++m_descriptorCounts.totalDescriptorSets;
}

void BuildContext::fill_buffer_raw(const Name buffName, const void* ptr, const MemorySize size)
{
   m_activePipelineStages = gapi::PipelineStage::Transfer;

   this->prepare_buffer(buffName, gapi::BufferUsage::TransferDst);

   std::vector<u8> data(size);
   std::memcpy(data.data(), ptr, size);

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::FillBuffer>{}, buffName, std::move(data));

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::copy_texture_to_buffer(const TextureRef srcTex, const BufferRef dstBuff)
{
   m_activePipelineStages = gapi::PipelineStage::Transfer;

   this->prepare_texture(srcTex, gapi::TextureState::TransferSrc, gapi::TextureUsage::TransferSrc);
   this->prepare_buffer(dstBuff, gapi::BufferUsage::TransferDst);

   this->add_command<detail::cmd::CopyTextureToBuffer>(srcTex, dstBuff);

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::copy_buffer_to_texture(const BufferRef srcBuff, const TextureRef dstTex)
{
   m_activePipelineStages = gapi::PipelineStage::Transfer;

   this->prepare_buffer(srcBuff, gapi::BufferUsage::TransferSrc);
   this->prepare_texture(dstTex, gapi::TextureState::TransferDst, gapi::TextureUsage::TransferDst);

   this->add_command<detail::cmd::CopyBufferToTexture>(srcBuff, dstTex);

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::copy_buffer(BufferRef srcBuffer, BufferRef dstBuffer)
{
   m_activePipelineStages = gapi::PipelineStage::Transfer;

   this->prepare_buffer(srcBuffer, gapi::BufferUsage::TransferSrc);
   this->prepare_buffer(dstBuffer, gapi::BufferUsage::TransferDst);

   this->add_command<detail::cmd::CopyBuffer>(srcBuffer, dstBuffer);

   m_workTypes |= gapi::WorkType::Transfer;
}

void BuildContext::copy_texture(TextureRef srcTex, TextureRef dstTex)
{
   m_activePipelineStages = gapi::PipelineStage::Transfer;

   this->prepare_texture(srcTex, gapi::TextureState::TransferSrc, gapi::TextureUsage::TransferSrc);
   this->prepare_texture(dstTex, gapi::TextureState::TransferDst, gapi::TextureUsage::TransferDst);

   this->add_command<detail::cmd::CopyTexture>(srcTex, dstTex);

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

void BuildContext::bind_rt_generation_shader(RayGenShaderName rayGenShader)
{
   m_workTypes |= gapi::WorkType::Graphics;
   m_rayTracingPipelineState.rayGenShader.emplace(rayGenShader);
   m_activePipelineStages = gapi::PipelineStage::RayGenerationShader;
}

void BuildContext::bind_rt_closest_hit_shader(RayClosestHitShaderName rayClosestHitShader)
{
   m_workTypes |= gapi::WorkType::Graphics;
   m_rayTracingPipelineState.rayClosestHitShaders.emplace_back(rayClosestHitShader);
   m_activePipelineStages = gapi::PipelineStage::ClosestHitShader;
}

void BuildContext::bind_rt_miss_shader(RayMissShaderName rayMissShader)
{
   m_workTypes |= gapi::WorkType::Graphics;
   m_rayTracingPipelineState.rayMissShaders.emplace_back(rayMissShader);
   m_activePipelineStages = gapi::PipelineStage::MissShader;
}

void BuildContext::bind_rt_shader_group(RayTracingShaderGroup group)
{
   m_rayTracingPipelineState.shaderGroups.emplace_back(group);
}

void BuildContext::set_rt_max_recursion_depth(const u32 maxRecursionDepth)
{
   m_rayTracingPipelineState.maxRecursion = maxRecursionDepth;
}

void BuildContext::bind_acceleration_structure(const u32 index, graphics_api::ray_tracing::AccelerationStructure& accelerationStructure)
{
   ++m_descriptorCounts.accelerationStructureCount;

   DescriptorInfo descriptor;
   descriptor.pipelineStages = m_activePipelineStages;
   descriptor.descriptorType = gapi::DescriptorType::AccelerationStructure;
   this->set_pipeline_state_descriptor(m_activePipelineStages, index, descriptor);

   this->set_descriptor<detail::descriptor::AccelerationStructure>(index, m_activePipelineStages, &accelerationStructure);
}

void BuildContext::trace_rays(const Vector3i dimensions)
{
   this->add_command<detail::cmd::BindRayTracingPipeline>(m_rayTracingPipelineState);

   this->handle_descriptor_bindings();

   for (auto&& pushConstant : m_pendingPushConstants) {
      this->add_command<detail::cmd::PushConstant>(std::move(pushConstant));
   }
   m_pendingPushConstants.clear();

   this->add_command<detail::cmd::TraceRays>(m_rayTracingPipelineState, dimensions);

   m_rayTracingPipelineState.reset();
   ++m_descriptorCounts.totalDescriptorSets;
}

Job BuildContext::build_job(PipelineCache& pipelineCache, ResourceStorage& storage, [[maybe_unused]] const Name jobName)
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

         TG_SET_DEBUG_NAME(commandList, create_command_list_name(jobName, frameIndex, enabledFlags));

         commandLists.emplace_back(std::move(commandList));
      }

      frames.emplace_back(std::move(descStorage), std::move(commandLists));
   }

   return {m_device, std::move(pool), frames, m_workTypes, m_flags};
}

void BuildContext::write_commands(ResourceStorage& storage, DescriptorStorage& descStorage, gapi::CommandList& cmdList,
                                  PipelineCache& cache, graphics_api::DescriptorPool* pool, const u32 frameIndex, const u32 enabledFlags)
{
   ApplyFlagConditionsPass applyConditionsPass(m_flags, enabledFlags);
   for (const auto& cmdVariant : m_commands) {
      visit_command(applyConditionsPass, cmdVariant);
   }

   this->reset_resource_states();

   BarrierInsertionPass barrierInsertionPass(*this);
   for (const auto& cmdVariant : applyConditionsPass.commands()) {
      visit_command(barrierInsertionPass, cmdVariant);
   }

   GenerateCommandListPass generatePass(*this, cache, descStorage, storage, cmdList, pool, frameIndex);
   for (const auto& cmdVariant : barrierInsertionPass.commands()) {
      visit_command(generatePass, cmdVariant);
   }
}

void BuildContext::create_resources(ResourceStorage& storage)
{
   for (const auto frameIndex : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
      for (const auto& declVariant : Values(m_declarations)) {
         std::visit(
            [this, frameIndex, &storage]<typename TDecl>(const TDecl& decl) {
               if constexpr (std::is_same_v<TDecl, detail::decl::Texture>) {
                  auto size = decl.texDims.value_or(m_screenSize);
                  if (decl.scaling.has_value()) {
                     size = Vector2i(Vector2(size) * decl.scaling.value());
                  }
                  auto texture = GAPI_CHECK(m_device.create_texture(decl.texFormat, {static_cast<u32>(size.x), static_cast<u32>(size.y)},
                                                                    decl.texUsageFlags, gapi::TextureState::Undefined,
                                                                    gapi::SampleCount::Single, decl.createMipLevels ? 0 : 1));
                  TG_SET_DEBUG_NAME(texture, create_object_name(decl.texName, frameIndex));

                  texture.sampler_properties() = decl.samplerProperties;
                  if (texture.sampler_properties().maxLod == 0.0f) {
                     texture.sampler_properties().maxLod = static_cast<float>(texture.mip_count());
                  }

                  if (decl.createMipLevels) {
                     for (const u32 mipLevel : Range(0u, texture.mip_count())) {
                        storage.register_texture_mip_view(decl.texName, mipLevel, frameIndex,
                                                          GAPI_CHECK(texture.create_mip_view(m_device, mipLevel)));
                     }
                  }

                  storage.register_texture(decl.texName, frameIndex, std::move(texture));
               } else if constexpr (std::is_same_v<TDecl, detail::decl::Buffer>) {
                  auto buffSize = decl.buffSize;
                  if (decl.scale.has_value()) {
                     buffSize = m_screenSize.x * m_screenSize.y * decl.scale.value() * buffSize;
                  }
                  auto buffer = GAPI_CHECK(m_device.create_buffer(decl.buffUsageFlags, buffSize));
                  TG_SET_DEBUG_NAME(buffer, create_object_name(decl.buffName, frameIndex));
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
   if (stages & gapi::PipelineStage::RayGenerationShader || stages & gapi::PipelineStage::ClosestHitShader ||
       stages & gapi::PipelineStage::MissShader || stages & gapi::PipelineStage::AnyHitShader ||
       stages & gapi::PipelineStage::IntersectionShader || stages & gapi::PipelineStage::CallableShader) {
      m_rayTracingPipelineState.descriptorState.descriptors[index] = info;
      m_rayTracingPipelineState.descriptorState.descriptorCount =
         std::max(m_rayTracingPipelineState.descriptorState.descriptorCount, index + 1);
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
   if (m_descriptorCounts.sampledTextureCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::ImageSampler, multiplier * m_descriptorCounts.sampledTextureCount);
   }
   if (m_descriptorCounts.textureCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::ImageOnly, multiplier * m_descriptorCounts.textureCount);
   }
   if (m_descriptorCounts.storageBufferCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::StorageBuffer, multiplier * m_descriptorCounts.storageBufferCount);
   }
   if (m_descriptorCounts.accelerationStructureCount != 0) {
      descriptorCounts.emplace_back(gapi::DescriptorType::AccelerationStructure,
                                    multiplier * m_descriptorCounts.accelerationStructureCount);
   }

   return GAPI_CHECK(m_device.create_descriptor_pool(descriptorCounts, multiplier * m_descriptorCounts.totalDescriptorSets));
}

graphics_api::WorkTypeFlags BuildContext::work_types() const
{
   return m_workTypes;
}

Vector2i BuildContext::screen_size() const
{
   return m_screenSize;
}

}// namespace triglav::render_core