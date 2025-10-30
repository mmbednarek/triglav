#include "BarrierInsertionPass.hpp"

#include "BuildContext.hpp"

namespace triglav::render_core {

namespace gapi = graphics_api;

namespace {
[[maybe_unused]] gapi::BufferAccess adjust_buffer_access_to_work_type(const gapi::BufferAccess access, const gapi::WorkTypeFlags work_types)
{
   using enum gapi::WorkType;
   using gapi::BufferAccess;
   // assuming every queue has transport bit

   switch (access) {
   case BufferAccess::UniformRead:
      [[fallthrough]];
   case BufferAccess::IndirectCmdRead:
      if (!(work_types & Compute) && !(work_types & Graphics)) {
         return BufferAccess::MemoryRead;
      }
      break;
   case BufferAccess::IndexRead:
      [[fallthrough]];
   case BufferAccess::VertexRead:
      [[fallthrough]];
   case BufferAccess::ShaderRead:
      if (!(work_types & Graphics)) {
         return BufferAccess::MemoryRead;
      }
      break;
   case BufferAccess::ShaderWrite:
      if (!(work_types & Graphics)) {
         return BufferAccess::MemoryWrite;
      }
      break;
   default:
      break;
   }

   return access;
}
}// namespace

BarrierInsertionPass::BarrierInsertionPass(BuildContext& context) :
    m_context(context)
{
}

void BarrierInsertionPass::visit(const detail::cmd::BindDescriptors& cmd)
{
   for (const auto& descriptor : cmd.descriptors) {
      std::visit(
         [this, stageFlags = descriptor->stages]<typename TDescriptor>(const TDescriptor& desc) {
            if constexpr (std::is_same_v<TDescriptor, detail::descriptor::RWTexture>) {
               this->setup_texture_barrier(desc.texRef, gapi::TextureState::General, stageFlags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SamplableTexture>) {
               this->setup_texture_barrier(desc.texRef, gapi::TextureState::ShaderRead, stageFlags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SampledTextureArray>) {
               for (const auto& texRef : desc.texRefs) {
                  this->setup_texture_barrier(texRef, gapi::TextureState::ShaderRead, stageFlags);
               }
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::Texture>) {
               this->setup_texture_barrier(desc.texRef, gapi::TextureState::ShaderRead, stageFlags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBuffer>) {
               this->setup_buffer_barrier(desc.buffRef, gapi::BufferAccess::UniformRead, stageFlags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBufferRange>) {
               this->setup_buffer_barrier(desc.buffRef, gapi::BufferAccess::UniformRead, stageFlags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBufferArray>) {
               for (const auto& buffRef : desc.buffers) {
                  this->setup_buffer_barrier(buffRef, gapi::BufferAccess::UniformRead, stageFlags);
               }
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::StorageBuffer>) {
               this->setup_buffer_barrier(desc.buffRef, gapi::BufferAccess::ShaderWrite, stageFlags);
            }
         },
         descriptor->descriptor);
   }

   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BindVertexBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.buffRef, gapi::BufferAccess::VertexRead, gapi::PipelineStage::VertexInput);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BindIndexBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.buffRef, gapi::BufferAccess::IndexRead, gapi::PipelineStage::VertexInput);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyTextureToBuffer& cmd)
{
   this->setup_texture_barrier(cmd.srcTexture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_buffer_barrier(cmd.dstBuffer, gapi::BufferAccess::TransferWrite, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyBufferToTexture& cmd)
{
   this->setup_buffer_barrier(cmd.srcBuffer, gapi::BufferAccess::TransferRead, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dstTexture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.srcBuffer, gapi::BufferAccess::TransferRead, gapi::PipelineStage::Transfer);
   this->setup_buffer_barrier(cmd.dstBuffer, gapi::BufferAccess::TransferWrite, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyTexture& cmd)
{
   this->setup_texture_barrier(cmd.srcTexture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dstTexture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyTextureRegion& cmd)
{
   this->setup_texture_barrier(cmd.srcTexture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dstTexture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BlitTexture& cmd)
{
   this->setup_texture_barrier(cmd.srcTexture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dstTexture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::FillBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.buffName, gapi::BufferAccess::TransferWrite, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BeginRenderPass& cmd)
{
   for (const Name target : cmd.renderTargets) {
      const auto& renderTarget = m_context.m_renderTargets.at(target);

      const auto isDepthTarget = renderTarget.flags & gapi::AttachmentAttribute::Depth;
      const auto isImageLoaded = renderTarget.flags & gapi::AttachmentAttribute::LoadImage;
      const auto isImageStored = renderTarget.flags & gapi::AttachmentAttribute::StoreImage;
      const auto targetStage = isDepthTarget ? gapi::PipelineStage::EarlyZ : gapi::PipelineStage::AttachmentOutput;
      const auto lastUsedStage = isDepthTarget ? gapi::PipelineStage::LateZ : gapi::PipelineStage::AttachmentOutput;
      const auto textureState =
         (isImageLoaded && !isImageStored) ? gapi::TextureState::ReadOnlyRenderTarget : gapi::TextureState::RenderTarget;

      this->setup_texture_barrier(target, textureState, targetStage, lastUsedStage);
   }

   m_isWithinRenderPass = true;
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::EndRenderPass& cmd)
{
   m_isWithinRenderPass = false;
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::DrawIndexedIndirectWithCount& cmd)
{
   this->setup_buffer_barrier(cmd.drawCallBuffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->setup_buffer_barrier(cmd.countBuffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::DrawIndirectWithCount& cmd)
{
   this->setup_buffer_barrier(cmd.drawCallBuffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->setup_buffer_barrier(cmd.countBuffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::DispatchIndirect& cmd)
{
   this->setup_buffer_barrier(cmd.indirectBuffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::ExportTexture& cmd)
{
   this->setup_texture_barrier(cmd.texName, cmd.state, cmd.pipelineStage);
   // no need to default visit
}

void BarrierInsertionPass::visit(const detail::cmd::ExportBuffer& /*cmd*/)
{
   // this->setup_buffer_barrier(cmd.buffName, adjust_buffer_access_to_work_type(cmd.access, m_context.work_types()),
   //                            cmd.pipelineStage);
   // no need to default visit
}

void BarrierInsertionPass::default_visit(const detail::Command& cmd)
{
   m_commands.push_back(cmd);
}

std::vector<detail::Command>& BarrierInsertionPass::commands()
{
   return m_commands;
}

void BarrierInsertionPass::setup_texture_barrier(const TextureRef texRef, const graphics_api::TextureState targetState,
                                                 const graphics_api::PipelineStageFlags targetStages,
                                                 const std::optional<graphics_api::PipelineStage> lastUsedStage)
{
   if (!std::holds_alternative<Name>(texRef) && !std::holds_alternative<FromLastFrame>(texRef) &&
       !std::holds_alternative<TextureMip>(texRef)) {
      return;
   }

   const Name texName = std::holds_alternative<Name>(texRef)            ? std::get<Name>(texRef)
                        : std::holds_alternative<FromLastFrame>(texRef) ? std::get<FromLastFrame>(texRef).name
                                                                        : std::get<TextureMip>(texRef).name;

   u32 baseMip = 0;
   u32 mipCount = 1;
   if (std::holds_alternative<TextureMip>(texRef)) {
      baseMip = std::get<TextureMip>(texRef).mipLevel;
   } else {
      auto& texDecl = m_context.declaration<detail::decl::Texture>(texName);
      if (texDecl.createMipLevels) {
         mipCount = calculate_mip_count(texDecl.dimensions(m_context.screen_size()));
      }
   }

   auto& tex = m_context.declaration<detail::decl::Texture>(texName);

   auto lateStage = targetStages;
   if (lastUsedStage.has_value()) {
      lateStage = *lastUsedStage;
   }
   const auto targetMemAccess = gapi::to_memory_access(targetState);

   u32 localCount = 1;
   for (u32 mipLevel = baseMip; mipLevel < (baseMip + mipCount); ++mipLevel) {
      if (mipLevel < (baseMip + mipCount - 1) && tex.currentStatePerMip[mipLevel] == tex.currentStatePerMip[mipLevel + 1] &&
          tex.lastStages[mipLevel] == tex.lastStages[mipLevel + 1]) {
         ++localCount;
         continue;
      }

      if (targetMemAccess == gapi::MemoryAccess::Write || tex.currentStatePerMip[mipLevel] != targetState ||
          tex.lastTextureBarrier == nullptr || tex.lastTextureBarrier->baseMipLevel != baseMip ||
          tex.lastTextureBarrier->mipLevelCount != mipCount) {
         if (tex.lastStages[mipLevel] != 0) {
            auto barrier =
               std::make_unique<TextureBarrier>(texName, tex.lastStages[mipLevel], targetStages, tex.currentStatePerMip[mipLevel],
                                                targetState, mipLevel - localCount + 1, localCount);
            tex.lastTextureBarrier =
               this->add_command_before_render_pass<detail::cmd::PlaceTextureBarrier>(std::move(barrier)).barrier.get();
         }

         for (u32 level = 0; level < localCount; ++level) {
            tex.lastStages[mipLevel - localCount + 1 + level] = lateStage;
         }
      } else if (tex.lastTextureBarrier != nullptr) {
         tex.lastTextureBarrier->dstStageFlags |= targetStages;
         for (u32 level = 0; level < localCount; ++level) {
            tex.lastStages[mipLevel - localCount + 1 + level] |= lateStage;
         }
      }

      for (u32 level = 0; level < localCount; ++level) {
         tex.currentStatePerMip[mipLevel - localCount + 1 + level] = targetState;
      }
      localCount = 1;
   }
}

void BarrierInsertionPass::setup_buffer_barrier(const BufferRef buffRef, const graphics_api::BufferAccess targetAccess,
                                                const graphics_api::PipelineStageFlags targetStages)
{
   if (!std::holds_alternative<Name>(buffRef) && !std::holds_alternative<FromLastFrame>(buffRef)) {
      return;
   }

   const Name buffName = std::holds_alternative<Name>(buffRef) ? std::get<Name>(buffRef) : std::get<FromLastFrame>(buffRef).name;
   auto& buffer = m_context.declaration<detail::decl::Buffer>(buffName);

   if (to_memory_access(targetAccess) == gapi::MemoryAccess::Write || to_memory_access(buffer.currentAccess) == gapi::MemoryAccess::Write ||
       buffer.lastBufferBarrier == nullptr) {
      if (buffer.lastStages != 0) {
         auto barrier = std::make_unique<BufferBarrier>(buffRef, buffer.lastStages, targetStages, buffer.currentAccess, targetAccess);
         buffer.lastBufferBarrier = this->add_command_before_render_pass<detail::cmd::PlaceBufferBarrier>(std::move(barrier)).barrier.get();
      }
      buffer.lastStages = targetStages;
   } else if (buffer.lastBufferBarrier != nullptr) {
      buffer.lastBufferBarrier->dstStageFlags |= targetStages;
      buffer.lastBufferBarrier->dstBufferAccess |= targetAccess;
      buffer.lastStages |= targetStages;
   }

   buffer.currentAccess = targetAccess;
}

}// namespace triglav::render_core
