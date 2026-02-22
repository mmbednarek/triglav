#include "AnimationManager.hpp"

#include "triglav/asset/Asset.hpp"

namespace triglav::renderer {

constexpr MemorySize STAGING_BUFFER_SIZE = 1024;
constexpr MemorySize STAGING_TIMESTAMP_OFFSET = 512;
constexpr MemorySize TIMESTAMP_BUFFER_SIZE = 4096;
constexpr MemorySize KEYFRAME_BUFFER_SIZE = 4096;

namespace gapi = graphics_api;

AnimationManager::AnimationManager(graphics_api::Device& device, resource::ResourceManager& resource_manager) :
    m_device(device),
    m_resource_manager(resource_manager),
    m_stage_buffer(GAPI_CHECK(m_device.create_buffer(
       gapi::BufferUsage::TransferSrc | gapi::BufferUsage::UniformBuffer | gapi::BufferUsage::HostVisible, STAGING_BUFFER_SIZE))),
    m_keyframe_buffer(
       GAPI_CHECK(m_device.create_buffer(gapi::BufferUsage::TransferDst | gapi::BufferUsage::StorageBuffer, KEYFRAME_BUFFER_SIZE))),
    m_timestamp_buffer(
       GAPI_CHECK(m_device.create_buffer(gapi::BufferUsage::TransferDst | gapi::BufferUsage::StorageBuffer, TIMESTAMP_BUFFER_SIZE)))
{
   m_base_time = std::chrono::system_clock::now();
}

AnimationID AnimationManager::start_animation(const AnimationName animation_name, const ObjectID target_object_id)
{
   const auto& animation = m_resource_manager.get(animation_name);

   std::vector<ChannelState> channel_states(animation.channels.size());
   std::ranges::transform(animation.channels, channel_states.begin(),
                          [offset = static_cast<u32>(m_keyframe_offset / sizeof(Vector4))](const auto& channel) mutable {
                             ChannelState state{
                                .first_keyframe = offset,
                                .last_keyframe = static_cast<u32>(offset + channel.keyframes.size() - 1),
                                .channel_type = static_cast<u32>(channel.type),
                             };
                             offset += channel.keyframes.size();
                             return state;
                          });

   const auto id = m_top_animation_id++;
   m_states[id] = AnimationState{
      .animation_name = animation_name,
      .target_object_id = target_object_id,
      .start_time = this->current_time(),
      .channels = std::move(channel_states),
   };

   MemorySize stage_keyframe_offset = 0;
   MemorySize stage_timestamp_offset = 0;

   {
      const auto mapping = m_stage_buffer.map_memory();
      assert(mapping.has_value());

      for (const auto& channel : animation.channels) {
         assert(channel.keyframes.size() == channel.timestamps.size());

         std::memcpy(static_cast<char*>(mapping->ptr()) + stage_keyframe_offset, channel.keyframes.data(),
                     channel.keyframes.size() * sizeof(Vector4));
         stage_keyframe_offset += channel.keyframes.size() * sizeof(Vector4);
         std::memcpy(static_cast<char*>(mapping->ptr()) + stage_timestamp_offset + STAGING_TIMESTAMP_OFFSET, channel.timestamps.data(),
                     channel.timestamps.size() * sizeof(float));
         stage_timestamp_offset += channel.timestamps.size() * sizeof(float);
      }
   }

   auto cmd_list = m_device.create_command_list();
   assert(cmd_list.has_value());
   GAPI_CHECK_STATUS(cmd_list->begin(graphics_api::SubmitType::OneTime));
   cmd_list->copy_buffer(m_stage_buffer, m_keyframe_buffer, 0, m_keyframe_offset, stage_keyframe_offset);
   cmd_list->copy_buffer(m_stage_buffer, m_timestamp_buffer, STAGING_TIMESTAMP_OFFSET, m_timestamp_offset, stage_timestamp_offset);
   GAPI_CHECK_STATUS(cmd_list->finish());

   m_keyframe_offset += stage_keyframe_offset;
   m_timestamp_offset += stage_timestamp_offset;

   GAPI_CHECK_STATUS(m_device.submit_command_list_one_time(*cmd_list));

   event_OnAnimationBegan.publish(id, m_states.at(id));
   return id;
}

float AnimationManager::current_time() const
{
   return static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - m_base_time).count()) /
          1000000.0f;
}

u32 AnimationManager::channel_count() const
{
   return static_cast<u32>(m_states.size());
}

graphics_api::Buffer& AnimationManager::keyframe_buffer()
{
   return m_keyframe_buffer;
}

const graphics_api::Buffer& AnimationManager::keyframe_buffer() const
{
   return m_keyframe_buffer;
}

graphics_api::Buffer& AnimationManager::timestamp_buffer()
{
   return m_timestamp_buffer;
}

const graphics_api::Buffer& AnimationManager::timestamp_buffer() const
{
   return m_timestamp_buffer;
}

const AnimationManager::StateContainer& AnimationManager::animation_states() const
{
   return m_states;
}

}// namespace triglav::renderer
