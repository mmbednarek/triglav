#pragma once

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
};

enum class AmbientOcclusionMethod
{
   Disabled,
   ScreenSpace,
   RayTraced,
};

[[nodiscard]] inline std::string_view ambient_occlusion_method_to_string(const AmbientOcclusionMethod method)
{
   using namespace std::string_view_literals;

   switch (method) {
   case AmbientOcclusionMethod::Disabled:
      return "Disabled"sv;
   case AmbientOcclusionMethod::ScreenSpace:
      return "Screen-Space"sv;
   case AmbientOcclusionMethod::RayTraced:
      return "Ray-Traced"sv;
   }

   return "Unknown"sv;
}

enum class AntialiasingMethod
{
   Disabled,
   FastApproximate,
};

[[nodiscard]] inline std::string_view antialiasing_method_to_string(const AntialiasingMethod method)
{
   using namespace std::string_view_literals;

   switch (method) {
   case AntialiasingMethod::Disabled:
      return "Disabled"sv;
   case AntialiasingMethod::FastApproximate:
      return "Fast-Approximate"sv;
   }

   return "Unknown"sv;
}

enum class ShadowCastingMethod
{
   ShadowMap,
   RayTracing,
};

[[nodiscard]] inline std::string_view shadow_casting_method_to_string(const ShadowCastingMethod method)
{
   using namespace std::string_view_literals;

   switch (method) {
   case ShadowCastingMethod::ShadowMap:
      return "Shadow-Map"sv;
   case ShadowCastingMethod::RayTracing:
      return "Ray-Tracing"sv;
   }

   return "Unknown"sv;
}

struct Config
{
   AmbientOcclusionMethod ambientOcclusion;
   AntialiasingMethod antialiasing;
   ShadowCastingMethod shadowCasting;
   bool isBloomEnabled;
   bool isUIHidden;
   bool isSmoothCameraEnabled;

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

   [[nodiscard]] const Config& config() const;

 private:
   const graphics_api::Device& m_device;
   Config m_config;
};

}// namespace triglav::renderer