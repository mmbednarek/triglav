#include "MeshLoader.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/io/File.hpp"

namespace triglav::resource {

render_objects::Mesh Loader<ResourceType::Mesh>::load_gpu(graphics_api::Device& device, MeshName /*name*/, const io::Path& path)
{
   const auto mesh = asset::load_mesh_data(path);

   graphics_api::BufferUsageFlags additional_usage_flags{graphics_api::BufferUsage::TransferSrc};
   if (device.enabled_features() & graphics_api::DeviceFeature::RayTracing) {
      additional_usage_flags |= graphics_api::BufferUsage::AccelerationStructureRead;
   }

   graphics_api::Buffer gpu_vertices = GAPI_CHECK(
      device.create_buffer(graphics_api::BufferUsage::VertexBuffer | graphics_api::BufferUsage::TransferDst | additional_usage_flags,
                           mesh.vertex_data.vertex_buffer.size()));
   GAPI_CHECK_STATUS(gpu_vertices.write_indirect(mesh.vertex_data.vertex_buffer.data(), mesh.vertex_data.vertex_buffer.size()));

   graphics_api::IndexArray gpu_indices{device, mesh.vertex_data.index_buffer.size(), additional_usage_flags};
   GAPI_CHECK_STATUS(gpu_indices.write(mesh.vertex_data.index_buffer.data(), mesh.vertex_data.index_buffer.size()));

   return {{std::move(gpu_vertices), std::move(gpu_indices), mesh.vertex_data.vertex_buffer.vertex_groups()}, mesh.bounding_box};
}

void Loader<ResourceType::Mesh>::collect_dependencies(std::set<ResourceName>& out_dependencies, const io::Path& path)
{
   const auto mesh_data = asset::load_mesh_data(path);
   for (const auto& range : mesh_data.vertex_data.vertex_buffer.vertex_groups()) {
      out_dependencies.insert(range.material_name);
   }
}

}// namespace triglav::resource