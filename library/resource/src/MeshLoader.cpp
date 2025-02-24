#include "MeshLoader.hpp"

#include "triglav/asset/Asset.hpp"
#include "triglav/geometry/Mesh.hpp"
#include "triglav/io/File.hpp"

#include <format>

namespace triglav::resource {

render_objects::Mesh Loader<ResourceType::Mesh>::load_gpu(graphics_api::Device& device, MeshName /*name*/, const io::Path& path,
                                                          const ResourceProperties& /*props*/)
{
   const auto meshFileHandle = io::open_file(path, io::FileOpenMode::Read);
   assert(meshFileHandle.has_value());

   const auto assetHeader = asset::decode_header(**meshFileHandle);
   assert(assetHeader.has_value());
   assert(assetHeader->type == ResourceType::Mesh);

   const auto mesh = asset::decode_mesh(**meshFileHandle);
   assert(mesh.has_value());

   graphics_api::BufferUsageFlags additionalUsageFlags{graphics_api::BufferUsage::TransferSrc};
   if (device.enabled_features() & graphics_api::DeviceFeature::RayTracing) {
      additionalUsageFlags |= graphics_api::BufferUsage::AccelerationStructureRead;
   }

   graphics_api::VertexArray<geometry::Vertex> gpuVertices{device, mesh->vertexData.vertices.size(), additionalUsageFlags};
   GAPI_CHECK_STATUS(gpuVertices.write(mesh->vertexData.vertices.data(), mesh->vertexData.vertices.size()));

   graphics_api::IndexArray gpuIndices{device, mesh->vertexData.indices.size(), additionalUsageFlags};
   GAPI_CHECK_STATUS(gpuIndices.write(mesh->vertexData.indices.data(), mesh->vertexData.indices.size()));

   return {{std::move(gpuVertices), std::move(gpuIndices)}, mesh->boundingBox, mesh->vertexData.ranges};
}

}// namespace triglav::resource