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
         [this, stage_flags = descriptor->stages]<typename TDescriptor>(const TDescriptor& desc) {
            if constexpr (std::is_same_v<TDescriptor, detail::descriptor::RWTexture>) {
               this->setup_texture_barrier(desc.tex_ref, gapi::TextureState::General, stage_flags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SamplableTexture>) {
               this->setup_texture_barrier(desc.tex_ref, gapi::TextureState::ShaderRead, stage_flags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SampledTextureArray>) {
               for (const auto& tex_ref : desc.tex_refs) {
                  this->setup_texture_barrier(tex_ref, gapi::TextureState::ShaderRead, stage_flags);
               }
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::Texture>) {
               this->setup_texture_barrier(desc.tex_ref, gapi::TextureState::ShaderRead, stage_flags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBuffer>) {
               this->setup_buffer_barrier(desc.buff_ref, gapi::BufferAccess::UniformRead, stage_flags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBufferRange>) {
               this->setup_buffer_barrier(desc.buff_ref, gapi::BufferAccess::UniformRead, stage_flags);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBufferArray>) {
               for (const auto& buff_ref : desc.buffers) {
                  this->setup_buffer_barrier(buff_ref, gapi::BufferAccess::UniformRead, stage_flags);
               }
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::StorageBuffer>) {
               this->setup_buffer_barrier(desc.buff_ref, gapi::BufferAccess::ShaderWrite, stage_flags);
            }
         },
         descriptor->descriptor);
   }

   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BindVertexBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.buff_ref, gapi::BufferAccess::VertexRead, gapi::PipelineStage::VertexInput);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BindIndexBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.buff_ref, gapi::BufferAccess::IndexRead, gapi::PipelineStage::VertexInput);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyTextureToBuffer& cmd)
{
   this->setup_texture_barrier(cmd.src_texture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_buffer_barrier(cmd.dst_buffer, gapi::BufferAccess::TransferWrite, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyBufferToTexture& cmd)
{
   this->setup_buffer_barrier(cmd.src_buffer, gapi::BufferAccess::TransferRead, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dst_texture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.src_buffer, gapi::BufferAccess::TransferRead, gapi::PipelineStage::Transfer);
   this->setup_buffer_barrier(cmd.dst_buffer, gapi::BufferAccess::TransferWrite, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyTexture& cmd)
{
   this->setup_texture_barrier(cmd.src_texture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dst_texture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::CopyTextureRegion& cmd)
{
   this->setup_texture_barrier(cmd.src_texture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dst_texture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BlitTexture& cmd)
{
   this->setup_texture_barrier(cmd.src_texture, gapi::TextureState::TransferSrc, gapi::PipelineStage::Transfer);
   this->setup_texture_barrier(cmd.dst_texture, gapi::TextureState::TransferDst, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::FillBuffer& cmd)
{
   this->setup_buffer_barrier(cmd.buff_name, gapi::BufferAccess::TransferWrite, gapi::PipelineStage::Transfer);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::BeginRenderPass& cmd)
{
   for (const Name target : cmd.render_targets) {
      const auto& render_target = m_context.m_render_targets.at(target);

      const auto is_depth_target = render_target.flags & gapi::AttachmentAttribute::Depth;
      const auto is_image_loaded = render_target.flags & gapi::AttachmentAttribute::LoadImage;
      const auto is_image_stored = render_target.flags & gapi::AttachmentAttribute::StoreImage;
      const auto target_stage = is_depth_target ? gapi::PipelineStage::EarlyZ : gapi::PipelineStage::AttachmentOutput;
      const auto last_used_stage = is_depth_target ? gapi::PipelineStage::LateZ : gapi::PipelineStage::AttachmentOutput;
      const auto texture_state =
         (is_image_loaded && !is_image_stored) ? gapi::TextureState::ReadOnlyRenderTarget : gapi::TextureState::RenderTarget;

      this->setup_texture_barrier(target, texture_state, target_stage, last_used_stage);
   }

   m_is_within_render_pass = true;
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::EndRenderPass& cmd)
{
   m_is_within_render_pass = false;
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::DrawIndexedIndirectWithCount& cmd)
{
   this->setup_buffer_barrier(cmd.draw_call_buffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->setup_buffer_barrier(cmd.count_buffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::DrawIndirectWithCount& cmd)
{
   this->setup_buffer_barrier(cmd.draw_call_buffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->setup_buffer_barrier(cmd.count_buffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::DispatchIndirect& cmd)
{
   this->setup_buffer_barrier(cmd.indirect_buffer, gapi::BufferAccess::IndirectCmdRead, gapi::PipelineStage::DrawIndirect);
   this->default_visit(cmd);
}

void BarrierInsertionPass::visit(const detail::cmd::ExportTexture& cmd)
{
   this->setup_texture_barrier(cmd.tex_name, cmd.state, cmd.pipeline_stage);
   // no need to default visit
}

void BarrierInsertionPass::visit(const detail::cmd::ExportBuffer& /*cmd*/)
{
   // this->setup_buffer_barrier(cmd.buff_name, adjust_buffer_access_to_work_type(cmd.access, m_context.work_types()),
   //                            cmd.pipeline_stage);
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

void BarrierInsertionPass::setup_texture_barrier(const TextureRef tex_ref, const graphics_api::TextureState target_state,
                                                 const graphics_api::PipelineStageFlags target_stages,
                                                 const std::optional<graphics_api::PipelineStage> last_used_stage)
{
   if (!std::holds_alternative<Name>(tex_ref) && !std::holds_alternative<FromLastFrame>(tex_ref) &&
       !std::holds_alternative<TextureMip>(tex_ref)) {
      return;
   }

   const Name tex_name = std::holds_alternative<Name>(tex_ref)            ? std::get<Name>(tex_ref)
                         : std::holds_alternative<FromLastFrame>(tex_ref) ? std::get<FromLastFrame>(tex_ref).name
                                                                          : std::get<TextureMip>(tex_ref).name;

   u32 base_mip = 0;
   u32 mip_count = 1;
   if (std::holds_alternative<TextureMip>(tex_ref)) {
      base_mip = std::get<TextureMip>(tex_ref).mip_level;
   } else {
      auto& tex_decl = m_context.declaration<detail::decl::Texture>(tex_name);
      if (tex_decl.create_mip_levels) {
         mip_count = calculate_mip_count(tex_decl.dimensions(m_context.screen_size()));
      }
   }

   auto& tex = m_context.declaration<detail::decl::Texture>(tex_name);

   auto late_stage = target_stages;
   if (last_used_stage.has_value()) {
      late_stage = *last_used_stage;
   }
   const auto target_mem_access = gapi::to_memory_access(target_state);

   u32 local_count = 1;
   for (u32 mip_level = base_mip; mip_level < (base_mip + mip_count); ++mip_level) {
      if (mip_level < (base_mip + mip_count - 1) && tex.current_state_per_mip[mip_level] == tex.current_state_per_mip[mip_level + 1] &&
          tex.last_stages[mip_level] == tex.last_stages[mip_level + 1]) {
         ++local_count;
         continue;
      }

      if (target_mem_access == gapi::MemoryAccess::Write || tex.current_state_per_mip[mip_level] != target_state ||
          tex.last_texture_barrier == nullptr || tex.last_texture_barrier->base_mip_level != base_mip ||
          tex.last_texture_barrier->mip_level_count != mip_count) {
         if (tex.last_stages[mip_level] != 0) {
            auto barrier =
               std::make_unique<TextureBarrier>(tex_name, tex.last_stages[mip_level], target_stages, tex.current_state_per_mip[mip_level],
                                                target_state, mip_level - local_count + 1, local_count);
            tex.last_texture_barrier =
               this->add_command_before_render_pass<detail::cmd::PlaceTextureBarrier>(std::move(barrier)).barrier.get();
         }

         for (u32 level = 0; level < local_count; ++level) {
            tex.last_stages[mip_level - local_count + 1 + level] = late_stage;
         }
      } else if (tex.last_texture_barrier != nullptr) {
         tex.last_texture_barrier->dst_stage_flags |= target_stages;
         for (u32 level = 0; level < local_count; ++level) {
            tex.last_stages[mip_level - local_count + 1 + level] |= late_stage;
         }
      }

      for (u32 level = 0; level < local_count; ++level) {
         tex.current_state_per_mip[mip_level - local_count + 1 + level] = target_state;
      }
      local_count = 1;
   }
}

void BarrierInsertionPass::setup_buffer_barrier(const BufferRef buff_ref, const graphics_api::BufferAccess target_access,
                                                const graphics_api::PipelineStageFlags target_stages)
{
   if (!std::holds_alternative<Name>(buff_ref) && !std::holds_alternative<FromLastFrame>(buff_ref)) {
      return;
   }

   const Name buff_name = std::holds_alternative<Name>(buff_ref) ? std::get<Name>(buff_ref) : std::get<FromLastFrame>(buff_ref).name;
   auto& buffer = m_context.declaration<detail::decl::Buffer>(buff_name);

   if (to_memory_access(target_access) == gapi::MemoryAccess::Write ||
       to_memory_access(buffer.current_access) == gapi::MemoryAccess::Write || buffer.last_buffer_barrier == nullptr) {
      if (buffer.last_stages != 0) {
         auto barrier = std::make_unique<BufferBarrier>(buff_ref, buffer.last_stages, target_stages, buffer.current_access, target_access);
         buffer.last_buffer_barrier =
            this->add_command_before_render_pass<detail::cmd::PlaceBufferBarrier>(std::move(barrier)).barrier.get();
      }
      buffer.last_stages = target_stages;
   } else if (buffer.last_buffer_barrier != nullptr) {
      buffer.last_buffer_barrier->dst_stage_flags |= target_stages;
      buffer.last_buffer_barrier->dst_buffer_access |= target_access;
      buffer.last_stages |= target_stages;
   }

   buffer.current_access = target_access;
}

}// namespace triglav::render_core
