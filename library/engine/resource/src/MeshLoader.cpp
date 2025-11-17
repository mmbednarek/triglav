#include "MeshLoader.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/io/File.hpp"

#include <format>

namespace triglav::resource {

render_objects::Mesh Loader<ResourceType::Mesh>::load_gpu(graphics_api::Device& device, MeshName /*name*/, const io::Path& path,
                                                          const ResourceProperties& /*props*/)
{
   const auto mesh_file_handle = io::open_file(path, io::FileOpenMode::Read);
   assert(mesh_file_handle.has_value());

   [[maybe_unused]]
   const auto asset_header = asset::decode_header(**mesh_file_handle);
   assert(asset_header.has_value());
   assert(asset_header->type == ResourceType::Mesh);

   const auto mesh = asset::decode_mesh(**mesh_file_handle);
   assert(mesh.has_value());

   graphics_api::BufferUsageFlags additional_usage_flags{graphics_api::BufferUsage::TransferSrc};
   if (device.enabled_features() & graphics_api::DeviceFeature::RayTracing) {
      additional_usage_flags |= graphics_api::BufferUsage::AccelerationStructureRead;
   }

   graphics_api::VertexArray<geometry::Vertex> gpu_vertices{device, mesh->vertex_data.vertices.size(), additional_usage_flags};
   GAPI_CHECK_STATUS(gpu_vertices.write(mesh->vertex_data.vertices.data(), mesh->vertex_data.vertices.size()));

   graphics_api::IndexArray gpu_indices{device, mesh->vertex_data.indices.size(), additional_usage_flags};
   GAPI_CHECK_STATUS(gpu_indices.write(mesh->vertex_data.indices.data(), mesh->vertex_data.indices.size()));

   return {{std::move(gpu_vertices), std::move(gpu_indices)}, mesh->bounding_box, mesh->vertex_data.ranges};
}

}// namespace triglav::resource