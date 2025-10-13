#pragma once

#include "triglav/String.hpp"
#include "triglav/event/Delegate.hpp"

#include <string_view>

namespace triglav::graphics_api {
class Device;
}

namespace triglav::renderer {

enum class ConfigProperty
{
   AmbientOcclusion,
   Antialiasing,
   ShadowCasting,
   IsBloomEnabled,
   IsUIHidden,
   IsSmoothCameraEnabled,
   IsRenderingParticles,
};

enum class AmbientOcclusionMethod
{
   Disabled,
   ScreenSpace,
   RayTraced,
};

[[nodiscard]] inline StringView ambient_occlusion_method_to_string(const AmbientOcclusionMethod method)
{
   using namespace string_literals;

   switch (method) {
   case AmbientOcclusionMethod::Disabled:
      return "Disabled"_strv;
   case AmbientOcclusionMethod::ScreenSpace:
      return "Screen-Space"_strv;
   case AmbientOcclusionMethod::RayTraced:
      return "Ray-Traced"_strv;
   }

   return "Unknown"_strv;
}

enum class AntialiasingMethod
{
   Disabled,
   FastApproximate,
};

[[nodiscard]] inline StringView antialiasing_method_to_string(const AntialiasingMethod method)
{
   using namespace string_literals;

   switch (method) {
   case AntialiasingMethod::Disabled:
      return "Disabled"_strv;
   case AntialiasingMethod::FastApproximate:
      return "Fast-Approximate"_strv;
   }

   return "Unknown"_strv;
}

enum class ShadowCastingMethod
{
   ShadowMap,
   RayTracing,
};

[[nodiscard]] inline StringView shadow_casting_method_to_string(const ShadowCastingMethod method)
{
   using namespace string_literals;

   switch (method) {
   case ShadowCastingMethod::ShadowMap:
      return "Shadow-Map"_strv;
   case ShadowCastingMethod::RayTracing:
      return "Ray-Tracing"_strv;
   }

   return "Unknown"_strv;
}

struct Config
{
   AmbientOcclusionMethod ambientOcclusion;
   AntialiasingMethod antialiasing;
   ShadowCastingMethod shadowCasting;
   bool isBloomEnabled;
   bool isUIHidden;
   bool isSmoothCameraEnabled;
   bool isRenderingParticles;

   [[nodiscard]] bool is_any_rt_feature_enabled() const
   {
      return this->ambientOcclusion == AmbientOcclusionMethod::RayTraced || this->shadowCasting == ShadowCastingMethod::RayTracing;
   }
};

class ConfigManager
{
 public:
   using Self = ConfigManager;

   TG_EVENT(OnPropertyChanged, ConfigProperty, const Config&)

   explicit ConfigManager(const graphics_api::Device& device);

   void toggle_ambient_occlusion();
   void toggle_antialiasing();
   void toggle_shadow_casting();
   void toggle_bloom();
   void toggle_ui_hidden();
   void toggle_smooth_camera();
   void toggle_rendering_particles();

   [[nodiscard]] const Config& config() const;

 private:
   const graphics_api::Device& m_device;
   Config m_config;
};

}// namespace triglav::renderer