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

namespace triglav::render_core {

namespace gapi = graphics_api;

namespace {

[[maybe_unused]] std::string create_command_list_name(const Name job_name, const u32 frame_index, const u32 enabled_flags)
{
   if (job_name == 0)
      return {};

   const auto resolved_name = resolve_name(job_name);
   if (resolved_name.empty())
      return {};

   return std::format("{}.frame{}.flags{}", resolved_name, frame_index, enabled_flags);
}

[[maybe_unused]] std::string create_object_name(const Name job_name, const u32 frame_index)
{
   if (job_name == 0)
      return {};

   const auto resolved_name = resolve_name(job_name);
   if (resolved_name.empty())
      return {};

   return std::format("{}.frame{}", resolved_name, frame_index);
}

}// namespace

BuildContext::BuildContext(graphics_api::Device& device, resource::ResourceManager& resource_manager, const Vector2i screen_size) :
    m_device(device),
    m_resource_manager(resource_manager),
    m_screen_size(screen_size)
{
}

void BuildContext::declare_texture(const Name tex_name, const Vector2i tex_dims, gapi::ColorFormat tex_format)
{
   this->add_declaration<detail::decl::Texture>(tex_name, tex_dims, tex_format);
}

void BuildContext::declare_screen_size_texture(const Name tex_name, const gapi::ColorFormat tex_format)
{
   this->add_declaration<detail::decl::Texture>(tex_name, std::nullopt, tex_format);
}

void BuildContext::declare_proportional_texture(const Name tex_name, const graphics_api::ColorFormat tex_format, const float scale,
                                                const bool create_mip_levels)
{
   this->add_declaration<detail::decl::Texture>(tex_name, std::nullopt, tex_format, gapi::TextureUsage::None, create_mip_levels, scale);
}

void BuildContext::declare_render_target(const Name rt_name, gapi::ColorFormat rt_format)
{
   this->m_render_targets.emplace(rt_name, detail::RenderTarget{gapi::ClearValue::color(gapi::ColorPalette::Black),
                                                                gapi::AttachmentAttribute::Color | gapi::AttachmentAttribute::ClearImage |
                                                                   gapi::AttachmentAttribute::StoreImage});
   this->add_declaration<detail::decl::Texture>(rt_name, std::nullopt, rt_format, gapi::TextureUsage::ColorAttachment);
}

void BuildContext::declare_depth_target(Name dt_name, graphics_api::ColorFormat rt_format)
{
   this->m_render_targets.emplace(dt_name, detail::RenderTarget{gapi::ClearValue::depth_stencil(1.0f, 0),
                                                                gapi::AttachmentAttribute::Depth | gapi::AttachmentAttribute::ClearImage});
   this->add_declaration<detail::decl::Texture>(dt_name, std::nullopt, rt_format, gapi::TextureUsage::DepthStencilAttachment);
}

void BuildContext::declare_proportional_depth_target(Name dt_name, graphics_api::ColorFormat rt_format, float scale)
{
   this->m_render_targets.emplace(dt_name, detail::RenderTarget{gapi::ClearValue::depth_stencil(1.0f, 0),
                                                                gapi::AttachmentAttribute::Depth | gapi::AttachmentAttribute::ClearImage});
   this->add_declaration<detail::decl::Texture>(dt_name, std::nullopt, rt_format, gapi::TextureUsage::DepthStencilAttachment, false, scale);
}

void BuildContext::declare_sized_render_target(const Name rt_name, const Vector2i rt_dims, gapi::ColorFormat rt_format)
{
   this->m_render_targets.emplace(rt_name, detail::RenderTarget{gapi::ClearValue::color(gapi::ColorPalette::Black),
                                                                gapi::AttachmentAttribute::Color | gapi::AttachmentAttribute::ClearImage |
                                                                   gapi::AttachmentAttribute::StoreImage});
   this->add_declaration<detail::decl::Texture>(rt_name, rt_dims, rt_format, gapi::TextureUsage::ColorAttachment);
}

void BuildContext::declare_sized_depth_target(Name dt_name, Vector2i dt_dims, graphics_api::ColorFormat rt_format)
{
   this->m_render_targets.emplace(dt_name, detail::RenderTarget{gapi::ClearValue::depth_stencil(1.0f, 0),
                                                                gapi::AttachmentAttribute::Depth | gapi::AttachmentAttribute::ClearImage});
   this->add_declaration<detail::decl::Texture>(dt_name, dt_dims, rt_format, gapi::TextureUsage::DepthStencilAttachment);
}

void BuildContext::declare_buffer(const Name buff_name, const MemorySize size)
{
   this->add_declaration<detail::decl::Buffer>(buff_name, size, gapi::BufferUsage::None);
}

void BuildContext::declare_staging_buffer(const Name buff_name, const MemorySize size)
{
   this->add_declaration<detail::decl::Buffer>(buff_name, size, gapi::BufferUsage::HostVisible);
}

void BuildContext::declare_proportional_buffer(const Name buff_name, const float scale, const MemorySize stride)
{
   this->add_declaration<detail::decl::Buffer>(buff_name, stride, gapi::BufferUsage::None, scale);
}

void BuildContext::bind_vertex_shader(VertexShaderName vs_name)
{
   m_work_types |= gapi::WorkType::Graphics;
   m_graphic_pipeline_state.vertex_shader = vs_name;
   m_active_pipeline_stages = gapi::PipelineStage::VertexShader;
}

void BuildContext::bind_fragment_shader(FragmentShaderName fs_name)
{
   m_work_types |= gapi::WorkType::Graphics;
   m_graphic_pipeline_state.fragment_shader = fs_name;
   m_active_pipeline_stages = gapi::PipelineStage::FragmentShader;
}

void BuildContext::bind_compute_shader(ComputeShaderName cs_name)
{
   m_work_types |= gapi::WorkType::Compute;
   m_compute_pipeline_state.compute_shader = cs_name;
   m_active_pipeline_stages = gapi::PipelineStage::ComputeShader;
}

void BuildContext::bind_rw_texture(const BindingIndex index, const TextureRef tex_ref)
{
   this->prepare_texture(tex_ref, gapi::TextureState::General, gapi::TextureUsage::Storage);

   ++m_descriptor_counts.storage_texture_count;

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::StorageImage;
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   this->set_descriptor<detail::descriptor::RWTexture>(index, m_active_pipeline_stages, tex_ref);
}

void BuildContext::bind_samplable_texture(const BindingIndex index, const TextureRef tex_ref)
{
   this->prepare_texture(tex_ref, gapi::TextureState::ShaderRead, gapi::TextureUsage::Sampled);

   ++m_descriptor_counts.sampled_texture_count;

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::ImageSampler;
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   this->set_descriptor<detail::descriptor::SamplableTexture>(index, m_active_pipeline_stages, tex_ref);
}

void BuildContext::bind_texture(const BindingIndex index, const TextureRef tex_ref)
{
   this->prepare_texture(tex_ref, gapi::TextureState::ShaderRead, gapi::TextureUsage::Sampled);

   ++m_descriptor_counts.texture_count;

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::ImageOnly;
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   this->set_descriptor<detail::descriptor::Texture>(index, m_active_pipeline_stages, tex_ref);
}

void BuildContext::bind_sampled_texture_array(const BindingIndex index, std::span<const TextureRef> tex_refs)
{
   for (const auto& ref : tex_refs) {
      this->prepare_texture(ref, gapi::TextureState::ShaderRead, gapi::TextureUsage::Sampled);
   }

   m_descriptor_counts.sampled_texture_count += static_cast<u32>(tex_refs.size());

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::ImageSampler;
   descriptor.descriptor_count = static_cast<u32>(tex_refs.size());
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   std::vector<TextureRef> texture_refs;
   texture_refs.reserve(tex_refs.size());
   std::ranges::copy(tex_refs, std::back_inserter(texture_refs));
   this->set_descriptor<detail::descriptor::SampledTextureArray>(index, m_active_pipeline_stages, std::move(texture_refs));
}

void BuildContext::bind_uniform_buffer(const BindingIndex index, const BufferRef buff_ref)
{
   this->prepare_buffer(buff_ref, gapi::BufferUsage::UniformBuffer);

   ++m_descriptor_counts.uniform_buffer_count;

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::UniformBuffer;
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   this->set_descriptor<detail::descriptor::UniformBuffer>(index, m_active_pipeline_stages, buff_ref);
}

void BuildContext::bind_uniform_buffer(BindingIndex index, BufferRef buff_ref, u32 offset, u32 size)
{
   this->prepare_buffer(buff_ref, gapi::BufferUsage::UniformBuffer);

   ++m_descriptor_counts.uniform_buffer_count;

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::UniformBuffer;
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   this->set_descriptor<detail::descriptor::UniformBufferRange>(index, m_active_pipeline_stages, buff_ref, offset, size);
}

void BuildContext::bind_uniform_buffers(const BindingIndex index, const std::span<const BufferRef> buffers)
{
   for (const auto& buffer : buffers) {
      this->prepare_buffer(buffer, gapi::BufferUsage::UniformBuffer);
   }

   m_descriptor_counts.uniform_buffer_count += static_cast<u32>(buffers.size());

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::UniformBuffer;
   descriptor.descriptor_count = static_cast<u32>(buffers.size());
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   std::vector<BufferRef> buffer_refs;
   buffer_refs.reserve(buffers.size());
   std::ranges::copy(buffers, std::back_inserter(buffer_refs));
   this->set_descriptor<detail::descriptor::UniformBufferArray>(index, m_active_pipeline_stages, std::move(buffer_refs));
}

void BuildContext::bind_storage_buffer(const BindingIndex index, const BufferRef buff_ref)
{
   this->prepare_buffer(buff_ref, gapi::BufferUsage::StorageBuffer);

   ++m_descriptor_counts.storage_buffer_count;

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::StorageBuffer;
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   this->set_descriptor<detail::descriptor::StorageBuffer>(index, m_active_pipeline_stages, buff_ref);
}

void BuildContext::push_constant_span(const std::span<const u8> buffer)
{
   if (m_active_pipeline_stages == gapi::PipelineStage::FragmentShader || m_active_pipeline_stages == gapi::PipelineStage::VertexShader) {
      m_graphic_pipeline_state.push_constants.emplace_back(m_active_pipeline_stages, static_cast<u32>(buffer.size()));
   } else if (m_active_pipeline_stages & gapi::PipelineStage::RayGenerationShader ||
              m_active_pipeline_stages & gapi::PipelineStage::MissShader ||
              m_active_pipeline_stages & gapi::PipelineStage::ClosestHitShader) {
      m_ray_tracing_pipeline_state.push_constants.emplace_back(m_active_pipeline_stages, static_cast<u32>(buffer.size()));
   }

   std::vector data(buffer.begin(), buffer.end());
   m_pending_push_constants.emplace_back(m_active_pipeline_stages, std::move(data));
}

void BuildContext::bind_vertex_layout(const VertexLayout& layout)
{
   m_graphic_pipeline_state.vertex_layout = layout;
}

void BuildContext::bind_vertex_buffer(const BufferRef buff_ref)
{
   m_active_pipeline_stages = gapi::PipelineStage::VertexInput;

   this->prepare_buffer(buff_ref, gapi::BufferUsage::VertexBuffer);

   this->add_command<detail::cmd::BindVertexBuffer>(buff_ref);
}

void BuildContext::bind_index_buffer(const BufferRef buff_ref)
{
   m_active_pipeline_stages = gapi::PipelineStage::VertexInput;

   this->prepare_buffer(buff_ref, gapi::BufferUsage::IndexBuffer);

   this->add_command<detail::cmd::BindIndexBuffer>(buff_ref);
}

void BuildContext::begin_render_pass_raw(const Name pass_name, const std::span<Name> render_targets)
{
   m_work_types |= gapi::WorkType::Graphics;

   for (const auto rt_name : render_targets) {
      const auto& rt_texture = this->declaration<detail::decl::Texture>(rt_name);
      const auto& render_target = m_render_targets.at(rt_name);

      if (render_target.flags & gapi::AttachmentAttribute::Depth) {
         m_graphic_pipeline_state.depth_target_format = rt_texture.tex_format;
      } else {
         m_graphic_pipeline_state.render_target_formats.emplace_back(rt_texture.tex_format);
      }
   }

   std::vector<Name> render_target_names(render_targets.size());
   std::ranges::copy(render_targets, render_target_names.begin());
   this->add_command<detail::cmd::BeginRenderPass>(pass_name, std::move(render_target_names));
}

void BuildContext::end_render_pass()
{
   m_graphic_pipeline_state.depth_target_format.reset();
   m_graphic_pipeline_state.render_target_formats.clear();
   this->add_command<detail::cmd::EndRenderPass>();
}

void BuildContext::clear_color(const Name target_name, const Vector4 color)
{
   m_render_targets.at(target_name).clear_value.value.emplace<gapi::Color>(color.x, color.y, color.z, color.w);
}

void BuildContext::clear_depth_stencil(Name target_name, float depth, u32 stencil)
{
   m_render_targets.at(target_name).clear_value.value.emplace<gapi::DepthStenctilValue>(depth, stencil);
}

void BuildContext::reset_timestamp_queries(const u32 offset, const u32 count)
{
   this->add_command<detail::cmd::ResetQueries>(offset, count, true);
}

void BuildContext::reset_pipeline_queries(u32 offset, u32 count)
{
   this->add_command<detail::cmd::ResetQueries>(offset, count, false);
}

void BuildContext::query_timestamp(const u32 index, const bool is_closing)
{
   this->add_command<detail::cmd::QueryTimestamp>(index, is_closing);
}

void BuildContext::begin_query(const u32 index)
{
   this->add_command<detail::cmd::BeginQuery>(index);
}

void BuildContext::end_query(const u32 index)
{
   this->add_command<detail::cmd::EndQuery>(index);
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
   graphics_api::DescriptorLayoutArray descriptor_layouts;
   descriptor_layouts.add_from_pipeline(pipeline);

   return storage.store_descriptor_array(GAPI_CHECK(pool.allocate_array(descriptor_layouts)));
}

void BuildContext::write_descriptor(ResourceStorage& storage, const graphics_api::DescriptorView& desc_view,
                                    const detail::cmd::BindDescriptors& descriptors, const u32 frame_index) const
{
   graphics_api::DescriptorWriter writer(m_device, desc_view);

   for (const auto [index, desc_variant] : Enumerate(descriptors.descriptors)) {
      std::visit(
         [this, index, frame_index, &writer, &storage]<typename TDescriptor>(const TDescriptor& desc) {
            if constexpr (std::is_same_v<TDescriptor, detail::descriptor::RWTexture>) {
               writer.set_storage_image_view(static_cast<u32>(index), this->resolve_texture_view_ref(storage, desc.tex_ref, frame_index));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SamplableTexture>) {
               const gapi::Texture& texture = this->resolve_texture_ref(storage, desc.tex_ref, frame_index);
               writer.set_sampled_texture(static_cast<u32>(index), texture,
                                          m_device.sampler_cache().find_sampler(texture.sampler_properties()));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::SampledTextureArray>) {
               std::vector<const gapi::Texture*> textures;
               textures.reserve(desc.tex_refs.size());
               for (const auto& tex_ref : desc.tex_refs) {
                  textures.push_back(&this->resolve_texture_ref(storage, tex_ref, frame_index));
               }
               writer.set_texture_array(static_cast<u32>(index), textures);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::Texture>) {
               writer.set_texture_view_only(static_cast<u32>(index), this->resolve_texture_view_ref(storage, desc.tex_ref, frame_index));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBuffer>) {
               writer.set_raw_uniform_buffer(static_cast<u32>(index), this->resolve_buffer_ref(storage, desc.buff_ref, frame_index));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBufferRange>) {
               writer.set_raw_uniform_buffer(static_cast<u32>(index), this->resolve_buffer_ref(storage, desc.buff_ref, frame_index),
                                             desc.offset, desc.size);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::UniformBufferArray>) {
               std::vector<const gapi::Buffer*> buffers;
               buffers.reserve(desc.buffers.size());
               for (const auto ref : desc.buffers) {
                  buffers.emplace_back(&this->resolve_buffer_ref(storage, ref, frame_index));
               }
               writer.set_uniform_buffer_array(static_cast<u32>(index), buffers);
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::StorageBuffer>) {
               writer.set_storage_buffer(static_cast<u32>(index), this->resolve_buffer_ref(storage, desc.buff_ref, frame_index));
            } else if constexpr (std::is_same_v<TDescriptor, detail::descriptor::AccelerationStructure>) {
               writer.set_acceleration_structure(static_cast<u32>(index), *desc.acceleration_structure);
            }
         },
         desc_variant->descriptor);
   }
}

void BuildContext::add_texture_usage(const Name tex_name, const graphics_api::TextureUsageFlags flags)
{
   if (!m_declarations.contains(tex_name)) {
      log_error("missing texture declaration: {}", resolve_name(tex_name));
      flush_logs();
      assert(false);
   }
   this->declaration<detail::decl::Texture>(tex_name).tex_usage_flags |= flags;
}

void BuildContext::add_buffer_usage(const Name buff_name, const graphics_api::BufferUsageFlags flags)
{
   this->declaration<detail::decl::Buffer>(buff_name).buff_usage_flags |= flags;
}

void BuildContext::set_bind_stages(graphics_api::PipelineStageFlags stages)
{
   m_active_pipeline_stages = stages;
}

bool BuildContext::is_ray_tracing_supported() const
{
   return m_device.enabled_features() & gapi::DeviceFeature::RayTracing;
}

void BuildContext::export_texture(const Name tex_name, const graphics_api::PipelineStage pipeline_stage,
                                  const graphics_api::TextureState state, const graphics_api::TextureUsageFlags flags)
{
   m_active_pipeline_stages = pipeline_stage;
   this->prepare_texture(tex_name, state, flags);
   this->add_command<detail::cmd::ExportTexture>(tex_name, pipeline_stage, state);
}

void BuildContext::export_buffer(const Name buff_name, const graphics_api::PipelineStage pipeline_stage,
                                 const graphics_api::BufferAccess access, const graphics_api::BufferUsageFlags flags)
{
   this->add_buffer_usage(buff_name, flags);
   this->add_command<detail::cmd::ExportBuffer>(buff_name, pipeline_stage, access);
}

graphics_api::SamplerProperties& BuildContext::texture_sampler_properties(const Name name)
{
   return this->declaration<detail::decl::Texture>(name).sampler_properties;
}

void BuildContext::set_sampler_properties(const Name name, const graphics_api::SamplerProperties& sampler_props)
{
   this->texture_sampler_properties(name) = sampler_props;
}

gapi::RenderingInfo BuildContext::create_rendering_info(ResourceStorage& storage, const detail::cmd::BeginRenderPass& begin_render_pass,
                                                        const u32 frame_index) const
{
   gapi::RenderingInfo info{};
   gapi::Resolution resolution{};

   for (const auto rt_name : begin_render_pass.render_targets) {
      const auto& render_target = m_render_targets.at(rt_name);

      gapi::RenderAttachment attachment{};
      attachment.texture = &storage.texture(rt_name, frame_index);
      attachment.state = gapi::TextureState::RenderTarget;
      attachment.clear_value = render_target.clear_value;
      attachment.flags = render_target.flags;

      resolution = attachment.texture->resolution();

      if (render_target.flags & gapi::AttachmentAttribute::Depth) {
         info.depth_attachment = attachment;
      } else {
         info.color_attachments.emplace_back(attachment);
      }
   }

   info.layer_count = 1;
   info.render_area_offset = {0, 0};
   info.render_area_extent = {resolution.width, resolution.height};

   return info;
}

const gapi::Texture& BuildContext::resolve_texture_ref(ResourceStorage& storage, TextureRef tex_ref, u32 frame_index) const
{
   return std::visit(
      [this, frame_index, &storage]<typename TVariant>(const TVariant& var) -> const gapi::Texture& {
         if constexpr (std::is_same_v<TVariant, Name>) {
            return storage.texture(var, frame_index);
         } else if constexpr (std::is_same_v<TVariant, External>) {
            return storage.texture(var.name, frame_index);
         } else if constexpr (std::is_same_v<TVariant, TextureMip>) {
            return storage.texture(var.name, frame_index);
         } else if constexpr (std::is_same_v<TVariant, FromLastFrame>) {
            return storage.texture(var.name, (FRAMES_IN_FLIGHT_COUNT + frame_index - 1) % FRAMES_IN_FLIGHT_COUNT);
         } else if constexpr (std::is_same_v<TVariant, TextureName>) {
            return m_resource_manager.get(var);
         } else if constexpr (std::is_same_v<TVariant, const graphics_api::Texture*>) {
            return *var;
         } else {
            assert(0);
         }
      },
      tex_ref);
}

const graphics_api::TextureView& BuildContext::resolve_texture_view_ref(ResourceStorage& storage, const TextureRef tex_ref,
                                                                        const u32 frame_index) const
{
   if (std::holds_alternative<TextureMip>(tex_ref)) {
      const auto& texture_mip = std::get<TextureMip>(tex_ref);
      return storage.texture_mip_view(texture_mip.name, texture_mip.mip_level, frame_index);
   }

   return this->resolve_texture_ref(storage, tex_ref, frame_index).view();
}

const graphics_api::Buffer& BuildContext::resolve_buffer_ref(ResourceStorage& storage, BufferRef buff_ref, u32 frame_index) const
{
   return std::visit(
      [frame_index, &storage]<typename TVariant>(const TVariant& var) -> const gapi::Buffer& {
         if constexpr (std::is_same_v<TVariant, Name>) {
            return storage.buffer(var, frame_index);
         } else if constexpr (std::is_same_v<TVariant, External>) {
            return storage.buffer(var.name, frame_index);
         } else if constexpr (std::is_same_v<TVariant, FromLastFrame>) {
            return storage.buffer(var.name, (FRAMES_IN_FLIGHT_COUNT + frame_index - 1) % FRAMES_IN_FLIGHT_COUNT);
         } else if constexpr (std::is_same_v<TVariant, const gapi::Buffer*>) {
            return *var;
         } else {
            assert(0);
         }
      },
      buff_ref);
}

void BuildContext::handle_pending_graphic_state()
{
   this->add_command<detail::cmd::BindGraphicsPipeline>(m_graphic_pipeline_state);

   for (auto&& push_constant : m_pending_push_constants) {
      this->add_command<detail::cmd::PushConstant>(std::move(push_constant));
   }
   m_pending_push_constants.clear();

   m_graphic_pipeline_state.descriptor_state.descriptor_count = 0;
   m_graphic_pipeline_state.vertex_layout.stride = 0;
   m_graphic_pipeline_state.vertex_layout.attributes.clear();
   m_graphic_pipeline_state.vertex_topology = graphics_api::VertexTopology::TriangleList;
   m_graphic_pipeline_state.depth_test_mode = graphics_api::DepthTestMode::Enabled;
   m_graphic_pipeline_state.push_constants.clear();
   m_graphic_pipeline_state.is_blending_enabled = true;

   ++m_descriptor_counts.total_descriptor_sets;

   this->handle_descriptor_bindings();
}

void BuildContext::prepare_texture(const TextureRef tex_ref, const gapi::TextureState state, const gapi::TextureUsageFlags usage)
{
   if (!std::holds_alternative<Name>(tex_ref) && !std::holds_alternative<FromLastFrame>(tex_ref) &&
       !std::holds_alternative<TextureMip>(tex_ref)) {
      return;
   }

   const Name tex_name = std::holds_alternative<Name>(tex_ref)            ? std::get<Name>(tex_ref)
                         : std::holds_alternative<FromLastFrame>(tex_ref) ? std::get<FromLastFrame>(tex_ref).name
                                                                          : std::get<TextureMip>(tex_ref).name;

   this->add_texture_usage(tex_name, usage);

   if (gapi::to_memory_access(state) == gapi::MemoryAccess::Read && m_render_targets.contains(tex_name)) {
      auto& render_target = m_render_targets.at(tex_name);
      render_target.flags |= gapi::AttachmentAttribute::StoreImage;
   }
   if (gapi::to_memory_access(state) == gapi::MemoryAccess::Write && m_render_targets.contains(tex_name)) {
      auto& render_target = m_render_targets.at(tex_name);
      render_target.flags |= gapi::AttachmentAttribute::LoadImage;
      render_target.flags.remove_flag(gapi::AttachmentAttribute::ClearImage);
   }
}

void BuildContext::prepare_buffer(const BufferRef buff_ref, const gapi::BufferUsage usage)
{
   if (!std::holds_alternative<Name>(buff_ref) && !std::holds_alternative<FromLastFrame>(buff_ref)) {
      return;
   }

   const Name buff_name = std::holds_alternative<Name>(buff_ref) ? std::get<Name>(buff_ref) : std::get<FromLastFrame>(buff_ref).name;

   this->add_buffer_usage(buff_name, usage);
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
               decl.current_state_per_mip.fill(graphics_api::TextureState::Undefined);
               decl.last_stages.fill(graphics_api::PipelineStage::Entrypoint);
               decl.last_texture_barrier = nullptr;
            } else if constexpr (std::is_same_v<TDecl, detail::decl::Buffer>) {
               decl.current_access = gapi::BufferAccess::None;
               decl.last_stages = graphics_api::PipelineStage::Entrypoint;
               decl.last_buffer_barrier = nullptr;
            }
         },
         decl);
   }
}

void BuildContext::set_vertex_topology(const graphics_api::VertexTopology topology)
{
   m_graphic_pipeline_state.vertex_topology = topology;
}

void BuildContext::set_depth_test_mode(const graphics_api::DepthTestMode mode)
{
   m_graphic_pipeline_state.depth_test_mode = mode;
}

void BuildContext::set_is_blending_enabled(const bool enabled)
{
   m_graphic_pipeline_state.is_blending_enabled = enabled;
}

void BuildContext::set_viewport(const Vector4 dimensions, const float min_depth, const float max_depth)
{
   this->add_command<detail::cmd::SetViewport>(dimensions, min_depth, max_depth);
}

void BuildContext::set_line_width(const float width)
{
   m_graphic_pipeline_state.line_width = width;
}

void BuildContext::draw_primitives(const u32 vertex_count, const u32 vertex_offset, u32 instance_count, u32 instance_offset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawPrimitives>(vertex_count, vertex_offset, instance_count, instance_offset);
}

void BuildContext::draw_indexed_primitives(const u32 index_count, const u32 index_offset, const u32 vertex_offset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedPrimitives>(index_count, index_offset, vertex_offset, 1u, 0u);
}

void BuildContext::draw_indexed_primitives(const u32 index_count, const u32 index_offset, const u32 vertex_offset, const u32 instance_count,
                                           const u32 instance_offset)
{
   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedPrimitives>(index_count, index_offset, vertex_offset, instance_count, instance_offset);
}

void BuildContext::draw_indexed_indirect_with_count(const BufferRef draw_call_buffer, const BufferRef count_buffer,
                                                    const u32 max_draw_calls, const u32 stride, const u32 count_buffer_offset)
{
   this->prepare_buffer(draw_call_buffer, gapi::BufferUsage::Indirect);
   this->prepare_buffer(count_buffer, gapi::BufferUsage::Indirect);

   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndexedIndirectWithCount>(draw_call_buffer, count_buffer, max_draw_calls, stride,
                                                                count_buffer_offset);
}

void BuildContext::draw_indirect_with_count(BufferRef draw_call_buffer, BufferRef count_buffer, u32 max_draw_calls, u32 stride)
{
   this->prepare_buffer(draw_call_buffer, gapi::BufferUsage::Indirect);
   this->prepare_buffer(count_buffer, gapi::BufferUsage::Indirect);

   this->handle_pending_graphic_state();
   this->add_command<detail::cmd::DrawIndirectWithCount>(draw_call_buffer, count_buffer, max_draw_calls, stride);
}

void BuildContext::draw_full_screen_quad()
{
   using namespace name_literals;
   this->bind_vertex_shader("shader/misc/full_screen.vshader"_rc);
   this->set_vertex_topology(gapi::VertexTopology::TriangleFan);
   this->draw_primitives(4, 0);
}

void BuildContext::dispatch(const Vector3i dims)
{
   assert(dims.x > 0 && dims.y > 0 && dims.z > 0);

   this->add_command<detail::cmd::BindComputePipeline>(m_compute_pipeline_state);
   m_compute_pipeline_state.descriptor_state.descriptor_count = 0;

   this->handle_descriptor_bindings();

   this->add_command<detail::cmd::Dispatch>(dims);

   ++m_descriptor_counts.total_descriptor_sets;
}

void BuildContext::dispatch_indirect(const BufferRef indirect_buffer)
{
   this->prepare_buffer(indirect_buffer, gapi::BufferUsage::Indirect);

   this->add_command<detail::cmd::BindComputePipeline>(m_compute_pipeline_state);
   m_compute_pipeline_state.descriptor_state.descriptor_count = 0;

   this->handle_descriptor_bindings();

   this->add_command<detail::cmd::DispatchIndirect>(indirect_buffer);

   ++m_descriptor_counts.total_descriptor_sets;
}

void BuildContext::fill_buffer_raw(const Name buff_name, const void* ptr, const MemorySize size)
{
   m_active_pipeline_stages = gapi::PipelineStage::Transfer;

   this->prepare_buffer(buff_name, gapi::BufferUsage::TransferDst);

   std::vector<u8> data(size);
   std::memcpy(data.data(), ptr, size);

   m_commands.emplace_back(std::in_place_type_t<detail::cmd::FillBuffer>{}, buff_name, std::move(data));

   m_work_types |= gapi::WorkType::Transfer;
}

void BuildContext::init_buffer_raw(const Name buff_name, const void* ptr, const MemorySize size)
{
   this->declare_buffer(buff_name, size);
   this->fill_buffer_raw(buff_name, ptr, size);
}

void BuildContext::copy_texture_to_buffer(const TextureRef src_tex, const BufferRef dst_buff)
{
   m_active_pipeline_stages = gapi::PipelineStage::Transfer;

   this->prepare_texture(src_tex, gapi::TextureState::TransferSrc, gapi::TextureUsage::TransferSrc);
   this->prepare_buffer(dst_buff, gapi::BufferUsage::TransferDst);

   this->add_command<detail::cmd::CopyTextureToBuffer>(src_tex, dst_buff);

   m_work_types |= gapi::WorkType::Transfer;
}

void BuildContext::copy_buffer_to_texture(const BufferRef src_buff, const TextureRef dst_tex)
{
   m_active_pipeline_stages = gapi::PipelineStage::Transfer;

   this->prepare_buffer(src_buff, gapi::BufferUsage::TransferSrc);
   this->prepare_texture(dst_tex, gapi::TextureState::TransferDst, gapi::TextureUsage::TransferDst);

   this->add_command<detail::cmd::CopyBufferToTexture>(src_buff, dst_tex);

   m_work_types |= gapi::WorkType::Transfer;
}

void BuildContext::copy_buffer(BufferRef src_buffer, BufferRef dst_buffer)
{
   m_active_pipeline_stages = gapi::PipelineStage::Transfer;

   this->prepare_buffer(src_buffer, gapi::BufferUsage::TransferSrc);
   this->prepare_buffer(dst_buffer, gapi::BufferUsage::TransferDst);

   this->add_command<detail::cmd::CopyBuffer>(src_buffer, dst_buffer);

   m_work_types |= gapi::WorkType::Transfer;
}

void BuildContext::copy_texture(TextureRef src_tex, TextureRef dst_tex)
{
   m_active_pipeline_stages = gapi::PipelineStage::Transfer;

   this->prepare_texture(src_tex, gapi::TextureState::TransferSrc, gapi::TextureUsage::TransferSrc);
   this->prepare_texture(dst_tex, gapi::TextureState::TransferDst, gapi::TextureUsage::TransferDst);

   this->add_command<detail::cmd::CopyTexture>(src_tex, dst_tex);

   m_work_types |= gapi::WorkType::Transfer;
}

void BuildContext::copy_texture_region(const TextureRef src_tex, const Vector2i src_offset, const TextureRef dst_tex,
                                       const Vector2i dst_offset, const Vector2i size)
{
   m_active_pipeline_stages = gapi::PipelineStage::Transfer;

   this->prepare_texture(src_tex, gapi::TextureState::TransferSrc, gapi::TextureUsage::TransferSrc);
   this->prepare_texture(dst_tex, gapi::TextureState::TransferDst, gapi::TextureUsage::TransferDst);

   this->add_command<detail::cmd::CopyTextureRegion>(src_tex, src_offset, dst_tex, dst_offset, size);

   m_work_types |= gapi::WorkType::Transfer;
}

void BuildContext::blit_texture(TextureRef src_tex, TextureRef dst_tex)
{
   m_active_pipeline_stages = gapi::PipelineStage::Transfer;

   this->prepare_texture(src_tex, gapi::TextureState::TransferSrc, gapi::TextureUsage::TransferSrc);
   this->prepare_texture(dst_tex, gapi::TextureState::TransferDst, gapi::TextureUsage::TransferDst);

   this->add_command<detail::cmd::BlitTexture>(src_tex, dst_tex);

   m_work_types |= gapi::WorkType::Graphics;
}

void BuildContext::declare_flag(const Name flag_name)
{
   m_flags.emplace_back(flag_name);
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

void BuildContext::bind_rt_generation_shader(RayGenShaderName ray_gen_shader)
{
   m_work_types |= gapi::WorkType::Graphics;
   m_ray_tracing_pipeline_state.ray_gen_shader.emplace(ray_gen_shader);
   m_active_pipeline_stages = gapi::PipelineStage::RayGenerationShader;
}

void BuildContext::bind_rt_closest_hit_shader(RayClosestHitShaderName ray_closest_hit_shader)
{
   m_work_types |= gapi::WorkType::Graphics;
   m_ray_tracing_pipeline_state.ray_closest_hit_shaders.emplace_back(ray_closest_hit_shader);
   m_active_pipeline_stages = gapi::PipelineStage::ClosestHitShader;
}

void BuildContext::bind_rt_miss_shader(RayMissShaderName ray_miss_shader)
{
   m_work_types |= gapi::WorkType::Graphics;
   m_ray_tracing_pipeline_state.ray_miss_shaders.emplace_back(ray_miss_shader);
   m_active_pipeline_stages = gapi::PipelineStage::MissShader;
}

void BuildContext::bind_rt_shader_group(RayTracingShaderGroup group)
{
   m_ray_tracing_pipeline_state.shader_groups.emplace_back(group);
}

void BuildContext::set_rt_max_recursion_depth(const u32 max_recursion_depth)
{
   m_ray_tracing_pipeline_state.max_recursion = max_recursion_depth;
}

void BuildContext::bind_acceleration_structure(const u32 index, graphics_api::ray_tracing::AccelerationStructure& acceleration_structure)
{
   ++m_descriptor_counts.acceleration_structure_count;

   DescriptorInfo descriptor;
   descriptor.pipeline_stages = m_active_pipeline_stages;
   descriptor.descriptor_type = gapi::DescriptorType::AccelerationStructure;
   this->set_pipeline_state_descriptor(m_active_pipeline_stages, index, descriptor);

   this->set_descriptor<detail::descriptor::AccelerationStructure>(index, m_active_pipeline_stages, &acceleration_structure);
}

void BuildContext::trace_rays(const Vector3i dimensions)
{
   this->add_command<detail::cmd::BindRayTracingPipeline>(m_ray_tracing_pipeline_state);

   this->handle_descriptor_bindings();

   for (auto&& push_constant : m_pending_push_constants) {
      this->add_command<detail::cmd::PushConstant>(std::move(push_constant));
   }
   m_pending_push_constants.clear();

   this->add_command<detail::cmd::TraceRays>(m_ray_tracing_pipeline_state, dimensions);

   m_ray_tracing_pipeline_state.reset();
   ++m_descriptor_counts.total_descriptor_sets;
}

Job BuildContext::build_job(PipelineCache& pipeline_cache, ResourceStorage& storage, [[maybe_unused]] const Name job_name)
{
   auto pool = this->create_descriptor_pool();

   std::vector<Job::Frame> frames;
   frames.reserve(FRAMES_IN_FLIGHT_COUNT);

   const auto flag_count = 1u << m_flags.size();

   this->create_resources(storage);

   for (const auto frame_index : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
      DescriptorStorage desc_storage;
      std::vector<gapi::CommandList> command_lists;

      for (const auto enabled_flags : Range(0u, flag_count)) {
         auto command_list = GAPI_CHECK(m_device.create_command_list(m_work_types));
         GAPI_CHECK_STATUS(command_list.begin(gapi::SubmitType::Normal));

         this->write_commands(storage, desc_storage, command_list, pipeline_cache, pool.has_value() ? &(*pool) : nullptr, frame_index,
                              enabled_flags);

         GAPI_CHECK_STATUS(command_list.finish());

         TG_SET_DEBUG_NAME(command_list, create_command_list_name(job_name, frame_index, enabled_flags));

         command_lists.emplace_back(std::move(command_list));
      }

      frames.emplace_back(std::move(desc_storage), std::move(command_lists));
   }

   return {m_device, std::move(pool), frames, m_work_types, m_flags};
}

void BuildContext::write_commands(ResourceStorage& storage, DescriptorStorage& desc_storage, gapi::CommandList& cmd_list,
                                  PipelineCache& cache, graphics_api::DescriptorPool* pool, const u32 frame_index, const u32 enabled_flags)
{
   ApplyFlagConditionsPass apply_conditions_pass(m_flags, enabled_flags);
   for (const auto& cmd_variant : m_commands) {
      visit_command(apply_conditions_pass, cmd_variant);
   }

   this->reset_resource_states();

   BarrierInsertionPass barrier_insertion_pass(*this);
   for (const auto& cmd_variant : apply_conditions_pass.commands()) {
      visit_command(barrier_insertion_pass, cmd_variant);
   }

   GenerateCommandListPass generate_pass(*this, cache, desc_storage, storage, cmd_list, pool, frame_index);
   for (const auto& cmd_variant : barrier_insertion_pass.commands()) {
      visit_command(generate_pass, cmd_variant);
   }
}

void BuildContext::create_resources(ResourceStorage& storage)
{
   for (const auto frame_index : Range(0, FRAMES_IN_FLIGHT_COUNT)) {
      for (const auto& decl_variant : Values(m_declarations)) {
         std::visit(
            [this, frame_index, &storage]<typename TDecl>(const TDecl& decl) {
               if constexpr (std::is_same_v<TDecl, detail::decl::Texture>) {
                  auto size = decl.tex_dims.value_or(m_screen_size);
                  if (decl.scaling.has_value()) {
                     size = Vector2i(Vector2(size) * decl.scaling.value());
                  }
                  auto texture = GAPI_CHECK(m_device.create_texture(decl.tex_format, {static_cast<u32>(size.x), static_cast<u32>(size.y)},
                                                                    decl.tex_usage_flags, gapi::TextureState::Undefined,
                                                                    gapi::SampleCount::Single, decl.create_mip_levels ? 0 : 1));
                  TG_SET_DEBUG_NAME(texture, create_object_name(decl.tex_name, frame_index));

                  texture.sampler_properties() = decl.sampler_properties;
                  if (texture.sampler_properties().max_lod == 0.0f) {
                     texture.sampler_properties().max_lod = static_cast<float>(texture.mip_count());
                  }

                  if (decl.create_mip_levels) {
                     for (const u32 mip_level : Range(0u, texture.mip_count())) {
                        storage.register_texture_mip_view(decl.tex_name, mip_level, frame_index,
                                                          GAPI_CHECK(texture.create_mip_view(m_device, mip_level)));
                     }
                  }

                  storage.register_texture(decl.tex_name, frame_index, std::move(texture));
               } else if constexpr (std::is_same_v<TDecl, detail::decl::Buffer>) {
                  auto buff_size = decl.buff_size;
                  if (decl.scale.has_value()) {
                     buff_size = static_cast<MemorySize>(m_screen_size.x * m_screen_size.y * decl.scale.value() * buff_size);
                  }
                  auto buffer = GAPI_CHECK(m_device.create_buffer(decl.buff_usage_flags, buff_size));
                  TG_SET_DEBUG_NAME(buffer, create_object_name(decl.buff_name, frame_index));
                  storage.register_buffer(decl.buff_name, frame_index, std::move(buffer));
               }
            },
            decl_variant);
      }
   }
}

void BuildContext::set_pipeline_state_descriptor(const graphics_api::PipelineStageFlags stages, const BindingIndex index,
                                                 const DescriptorInfo& info)
{
   assert(index < MAX_DESCRIPTOR_COUNT);
   if (stages & gapi::PipelineStage::FragmentShader || stages & gapi::PipelineStage::VertexShader) {
      m_graphic_pipeline_state.descriptor_state.descriptors[index] = info;
      m_graphic_pipeline_state.descriptor_state.descriptor_count =
         std::max(m_graphic_pipeline_state.descriptor_state.descriptor_count, index + 1);
   }
   if (stages & gapi::PipelineStage::ComputeShader) {
      m_compute_pipeline_state.descriptor_state.descriptors[index] = info;
      m_compute_pipeline_state.descriptor_state.descriptor_count =
         std::max(m_compute_pipeline_state.descriptor_state.descriptor_count, index + 1);
   }
   if (stages & gapi::PipelineStage::RayGenerationShader || stages & gapi::PipelineStage::ClosestHitShader ||
       stages & gapi::PipelineStage::MissShader || stages & gapi::PipelineStage::AnyHitShader ||
       stages & gapi::PipelineStage::IntersectionShader || stages & gapi::PipelineStage::CallableShader) {
      m_ray_tracing_pipeline_state.descriptor_state.descriptors[index] = info;
      m_ray_tracing_pipeline_state.descriptor_state.descriptor_count =
         std::max(m_ray_tracing_pipeline_state.descriptor_state.descriptor_count, index + 1);
   }
}

std::optional<gapi::DescriptorPool> BuildContext::create_descriptor_pool() const
{
   if (m_descriptor_counts.total_descriptor_sets == 0) {
      return std::nullopt;
   }

   const auto multiplier = FRAMES_IN_FLIGHT_COUNT * this->flag_variation_count();

   std::vector<std::pair<gapi::DescriptorType, u32>> descriptor_counts;
   if (m_descriptor_counts.storage_texture_count != 0) {
      descriptor_counts.emplace_back(gapi::DescriptorType::StorageImage, multiplier * m_descriptor_counts.storage_texture_count);
   }
   if (m_descriptor_counts.uniform_buffer_count != 0) {
      descriptor_counts.emplace_back(gapi::DescriptorType::UniformBuffer, multiplier * m_descriptor_counts.uniform_buffer_count);
   }
   if (m_descriptor_counts.sampled_texture_count != 0) {
      descriptor_counts.emplace_back(gapi::DescriptorType::ImageSampler, multiplier * m_descriptor_counts.sampled_texture_count);
   }
   if (m_descriptor_counts.texture_count != 0) {
      descriptor_counts.emplace_back(gapi::DescriptorType::ImageOnly, multiplier * m_descriptor_counts.texture_count);
   }
   if (m_descriptor_counts.storage_buffer_count != 0) {
      descriptor_counts.emplace_back(gapi::DescriptorType::StorageBuffer, multiplier * m_descriptor_counts.storage_buffer_count);
   }
   if (m_descriptor_counts.acceleration_structure_count != 0) {
      descriptor_counts.emplace_back(gapi::DescriptorType::AccelerationStructure,
                                     multiplier * m_descriptor_counts.acceleration_structure_count);
   }

   return GAPI_CHECK(m_device.create_descriptor_pool(descriptor_counts, multiplier * m_descriptor_counts.total_descriptor_sets));
}

graphics_api::WorkTypeFlags BuildContext::work_types() const
{
   return m_work_types;
}

Vector2i BuildContext::screen_size() const
{
   return m_screen_size;
}

}// namespace triglav::render_core