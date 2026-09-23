#include "ShadowMapManager.hpp"

namespace triglav::renderer {

ShadowMapManager::ShadowMapManager(const ViewContext& ctx) :
    TG_CONNECT(ctx, OnViewUpdated, on_view_updated)
{
   this->on_view_updated(ctx.camera());
}

void ShadowMapManager::on_view_updated(const Camera& camera)
{
   auto sm_props1 = camera.calculate_shadow_map(m_directional_light_orientation, 32.0f, 120.0f);
   m_directional_shadow_map_cameras[0] = OrthoCamera::from_properties(sm_props1);
   event_OnShadowMapChanged.publish(0, m_directional_shadow_map_cameras[0]);

   auto sm_props2 = camera.calculate_shadow_map(m_directional_light_orientation, 72.0f, 192.0f);
   m_directional_shadow_map_cameras[1] = OrthoCamera::from_properties(sm_props2);
   event_OnShadowMapChanged.publish(1, m_directional_shadow_map_cameras[1]);

   auto sm_props3 = camera.calculate_shadow_map(m_directional_light_orientation, 180.0f, 256.0f);
   m_directional_shadow_map_cameras[2] = OrthoCamera::from_properties(sm_props3);
   event_OnShadowMapChanged.publish(2, m_directional_shadow_map_cameras[2]);
}

const OrthoCamera& ShadowMapManager::shadow_map_camera(const u32 index) const
{
   assert(index <= m_directional_shadow_map_cameras.size());
   return m_directional_shadow_map_cameras[index];
}

u32 ShadowMapManager::directional_shadow_map_count() const
{
   return m_directional_shadow_map_cameras.size();
}

}// namespace triglav::renderer
