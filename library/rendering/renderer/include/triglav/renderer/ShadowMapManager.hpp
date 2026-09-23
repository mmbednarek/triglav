#pragma once

#include "triglav/Event.hpp"
#include "triglav/Math.hpp"

#include "Camera.hpp"
#include "OrthoCamera.hpp"
#include "ViewContext.hpp"

namespace triglav::renderer {

class ShadowMapManager
{
 public:
   TG_TAG_CLASS(triglav::renderer::ShadowMapManager)

   TG_EVENT(OnShadowMapChanged, u32, const OrthoCamera&)

   explicit ShadowMapManager(const ViewContext& ctx);

   void on_view_updated(const Camera& camera);

   [[nodiscard]] const OrthoCamera& shadow_map_camera(u32 index) const;
   [[nodiscard]] u32 directional_shadow_map_count() const;

 private:
   Quaternion m_directional_light_orientation{Vector3{-0.3f, 0.0f, 1.62f}};
   std::array<OrthoCamera, 3> m_directional_shadow_map_cameras{};

   TG_SINK(OnViewUpdated);
};

}// namespace triglav::renderer
