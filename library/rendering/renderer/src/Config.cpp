#include "Config.hpp"

#include "triglav/graphics_api/Device.hpp"

namespace triglav::renderer {

namespace {

Config get_default_config(const graphics_api::Device& device)
{
   const bool is_ray_tracing_enabled = device.enabled_features() & graphics_api::DeviceFeature::RayTracing;

   Config result{};
   result.ambient_occlusion = is_ray_tracing_enabled ? AmbientOcclusionMethod::RayTraced : AmbientOcclusionMethod::ScreenSpace;
   result.antialiasing = AntialiasingMethod::FastApproximate;
   result.shadow_casting = is_ray_tracing_enabled ? ShadowCastingMethod::RayTracing : ShadowCastingMethod::ShadowMap;
   result.is_bloom_enabled = true;
   result.is_uihidden = false;
   result.is_smooth_camera_enabled = true;
   result.is_rendering_particles = true;

   return result;
}

AmbientOcclusionMethod next_ambient_occlusion_method(const AmbientOcclusionMethod ao_method, const bool is_ray_tracing_enabled)
{
   switch (ao_method) {
   case AmbientOcclusionMethod::Disabled:
      return AmbientOcclusionMethod::ScreenSpace;
   case AmbientOcclusionMethod::ScreenSpace:
      if (is_ray_tracing_enabled) {
         return AmbientOcclusionMethod::RayTraced;
      } else {
         return AmbientOcclusionMethod::Disabled;
      }
   default:
      return AmbientOcclusionMethod::Disabled;
   }
}

}// namespace

ConfigManager::ConfigManager(const graphics_api::Device& device) :
    m_device(device),
    m_config(get_default_config(device))
{
}

void ConfigManager::toggle_ambient_occlusion()
{
   m_config.ambient_occlusion =
      next_ambient_occlusion_method(m_config.ambient_occlusion, m_device.enabled_features() & graphics_api::DeviceFeature::RayTracing);
   event_OnPropertyChanged.publish(ConfigProperty::AmbientOcclusion, m_config);
}

void ConfigManager::toggle_antialiasing()
{
   m_config.antialiasing =
      m_config.antialiasing == AntialiasingMethod::Disabled ? AntialiasingMethod::FastApproximate : AntialiasingMethod::Disabled;
   event_OnPropertyChanged.publish(ConfigProperty::Antialiasing, m_config);
}

void ConfigManager::toggle_shadow_casting()
{
   if (!(m_device.enabled_features() & graphics_api::DeviceFeature::RayTracing)) {
      assert(m_config.shadow_casting == ShadowCastingMethod::ShadowMap);
      return;
   }

   m_config.shadow_casting =
      m_config.shadow_casting == ShadowCastingMethod::RayTracing ? ShadowCastingMethod::ShadowMap : ShadowCastingMethod::RayTracing;
   event_OnPropertyChanged.publish(ConfigProperty::ShadowCasting, m_config);
}

void ConfigManager::toggle_bloom()
{
   m_config.is_bloom_enabled = !m_config.is_bloom_enabled;
   event_OnPropertyChanged.publish(ConfigProperty::IsBloomEnabled, m_config);
}

void ConfigManager::toggle_ui_hidden()
{
   m_config.is_uihidden = !m_config.is_uihidden;
   event_OnPropertyChanged.publish(ConfigProperty::IsUIHidden, m_config);
}

void ConfigManager::toggle_smooth_camera()
{
   m_config.is_smooth_camera_enabled = !m_config.is_smooth_camera_enabled;
   event_OnPropertyChanged.publish(ConfigProperty::IsSmoothCameraEnabled, m_config);
}

void ConfigManager::toggle_rendering_particles()
{
   m_config.is_rendering_particles = !m_config.is_rendering_particles;
   event_OnPropertyChanged.publish(ConfigProperty::IsRenderingParticles, m_config);
}

const Config& ConfigManager::config() const
{
   return m_config;
}

}// namespace triglav::renderer
