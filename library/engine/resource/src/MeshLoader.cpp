#include "MeshLoader.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/io/File.hpp"

#include <format>

namespace triglav::resource {

namespace {

geometry::MeshData load_mesh_data(const io::Path& path)
{
   const auto mesh_file_handle = io::open_file(path, io::FileMode::Read);
   assert(mesh_file_handle.has_value());

   [[maybe_unused]]
   const auto asset_header = asset::decode_header(**mesh_file_handle);
   assert(asset_header.has_value());
   assert(asset_header->type == ResourceType::Mesh);

   const auto mesh = asset::decode_mesh(**mesh_file_handle, asset_header->version);
   assert(mesh.has_value());
   return *mesh;
}

}// namespace

render_objects::Mesh Loader<ResourceType::Mesh>::load_gpu(graphics_api::Device& device, MeshName /*name*/, const io::Path& path)
{
   const auto mesh = load_mesh_data(path);

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
   const auto mesh_data = load_mesh_data(path);
   for (const auto& range : mesh_data.vertex_data.vertex_buffer.vertex_groups()) {
      out_dependencies.insert(range.material_name);
   }
}

}// namespace triglav::resource