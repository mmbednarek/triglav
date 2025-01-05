#include "ModelLoader.hpp"

#include "triglav/geometry/Mesh.hpp"

#include <algorithm>
#include <format>

namespace triglav::resource {

render_objects::Model Loader<ResourceType::Model>::load_gpu(graphics_api::Device& device, ModelName /*name*/, const io::Path& path,
                                                            const ResourceProperties& /*props*/)
{
   const auto objMesh = geometry::Mesh::from_file(path);
   objMesh.triangulate();
   objMesh.recalculate_tangents();

   graphics_api::BufferUsageFlags additionalUsageFlags{graphics_api::BufferUsage::TransferSrc};
   if (device.enabled_features() & graphics_api::DeviceFeature::RayTracing) {
      additionalUsageFlags |= graphics_api::BufferUsage::AccelerationStructureRead;
   }

   auto deviceMesh = objMesh.upload_to_device(device, additionalUsageFlags);

   std::vector<render_objects::MaterialRange> ranges{};
   ranges.reserve(deviceMesh.ranges.size());
   std::ranges::transform(deviceMesh.ranges, std::back_inserter(ranges), [](const geometry::MaterialRange& range) {
      return render_objects::MaterialRange{range.offset, range.size,
                                           make_rc_name(std::format("{}.mat", range.materialName)).operator MaterialName()};
   });

   return render_objects::Model{std::move(deviceMesh.mesh), objMesh.calculate_bounding_box(), std::move(ranges)};
}

}// namespace triglav::resource