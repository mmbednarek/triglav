#include "Config.hpp"

#include "triglav/graphics_api/Device.hpp"

namespace triglav::renderer {

namespace {

Config get_default_config(const graphics_api::Device& device)
{
   const bool isRayTracingEnabled = device.enabled_features() & graphics_api::DeviceFeature::RayTracing;

   Config result{};
   result.ambientOcclusion = isRayTracingEnabled ? AmbientOcclusionMethod::RayTraced : AmbientOcclusionMethod::ScreenSpace;
   result.antialiasing = AntialiasingMethod::FastApproximate;
   result.shadowCasting = isRayTracingEnabled ? ShadowCastingMethod::RayTracing : ShadowCastingMethod::ShadowMap;
   result.isBloomEnabled = true;
   result.isUIHidden = false;
   result.isSmoothCameraEnabled = true;

   return result;
}

AmbientOcclusionMethod next_ambient_occlusion_method(const AmbientOcclusionMethod aoMethod, const bool isRayTracingEnabled)
{
   switch (aoMethod) {
   case AmbientOcclusionMethod::Disabled:
      return AmbientOcclusionMethod::ScreenSpace;
   case AmbientOcclusionMethod::ScreenSpace:
      if (isRayTracingEnabled) {
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
   m_config.ambientOcclusion =
      next_ambient_occlusion_method(m_config.ambientOcclusion, m_device.enabled_features() & graphics_api::DeviceFeature::RayTracing);
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
      assert(m_config.shadowCasting == ShadowCastingMethod::ShadowMap);
      return;
   }

   m_config.shadowCasting =
      m_config.shadowCasting == ShadowCastingMethod::RayTracing ? ShadowCastingMethod::ShadowMap : ShadowCastingMethod::RayTracing;
   event_OnPropertyChanged.publish(ConfigProperty::ShadowCasting, m_config);
}

void ConfigManager::toggle_bloom()
{
   m_config.isBloomEnabled = !m_config.isBloomEnabled;
   event_OnPropertyChanged.publish(ConfigProperty::IsBloomEnabled, m_config);
}

void ConfigManager::toggle_ui_hidden()
{
   m_config.isUIHidden = !m_config.isUIHidden;
   event_OnPropertyChanged.publish(ConfigProperty::IsUIHidden, m_config);
}

void ConfigManager::toggle_smooth_camera()
{
   m_config.isSmoothCameraEnabled = !m_config.isSmoothCameraEnabled;
   event_OnPropertyChanged.publish(ConfigProperty::IsSmoothCameraEnabled, m_config);
}

const Config& ConfigManager::config() const
{
   return m_config;
}

}// namespace triglav::renderer
