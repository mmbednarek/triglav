#include "ModelLoader.h"

#include "triglav/geometry/Mesh.h"

#include <algorithm>
#include <format>

namespace triglav::resource {

render_core::Model Loader<ResourceType::Model>::load_gpu(graphics_api::Device& device, const io::Path& path,
                                                         const ResourceProperties& props)
{
   const auto objMesh = geometry::Mesh::from_file(path);
   objMesh.triangulate();
   objMesh.recalculate_tangents();

   graphics_api::BufferUsageFlags additionalUsageFlags{};
   if (device.enabled_features() & graphics_api::DeviceFeature::RayTracing) {
      additionalUsageFlags |= graphics_api::BufferUsage::AccelerationStructureRead;
   }

   auto deviceMesh = objMesh.upload_to_device(device, additionalUsageFlags);

   std::vector<render_core::MaterialRange> ranges{};
   ranges.resize(deviceMesh.ranges.size());
   std::transform(deviceMesh.ranges.begin(), deviceMesh.ranges.end(), ranges.begin(), [](const geometry::MaterialRange& range) {
      return render_core::MaterialRange{range.offset, range.size, make_rc_name(std::format("{}.mat", range.materialName))};
   });

   return render_core::Model{std::move(deviceMesh.mesh), objMesh.calculate_bounding_box(), std::move(ranges)};
}

}// namespace triglav::resource