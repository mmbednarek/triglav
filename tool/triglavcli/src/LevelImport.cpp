#include "LevelImport.hpp"

#include "MeshImport.hpp"
#include "ProjectConfig.hpp"
#include "ResourceList.hpp"

#include "triglav/gltf/Glb.hpp"
#include "triglav/gltf/MeshLoad.hpp"
#include "triglav/world/Level.hpp"

#include <fmt/core.h>
#include <utility>

namespace triglav::tool::cli {

using namespace name_literals;

class LevelImporter
{
 public:
   LevelImporter(LevelImportProps props, ProjectInfo&& projectInfo, gltf::GlbResource&& glbResource) :
       m_props(std::move(props)),
       m_projectInfo(std::move(projectInfo)),
       m_glbFile(std::move(glbResource))
   {
   }

   [[nodiscard]] std::optional<Name> import_mesh(const std::optional<std::string>& name, const u32 meshID)
   {
      if (m_importedMeshes.contains(meshID)) {
         return m_importedMeshes.at(meshID);
      }

      auto meshNameStr = name.value_or(fmt::format("{}.mesh{}", m_props.srcPath.basename(), meshID));
      for (char& ch : meshNameStr) {
         ch = static_cast<char>(std::tolower(ch));
      }

      auto importPath = m_projectInfo.default_import_path(ResourceType::Mesh, meshNameStr);
      const auto rcName = make_rc_name(importPath);
      assert(rcName.type() == ResourceType::Mesh);

      auto gltfMesh = gltf::mesh_from_document(*m_glbFile.document, meshID, m_glbFile.bufferManager);
      write_mesh_to_file(gltfMesh, m_projectInfo.content_path(importPath));
      m_importedMeshes.emplace(meshID, rcName.name());

      fmt::print(stderr, "triglav-cli: Importing mesh to {}\n", importPath);

      if (!add_resource_to_index(importPath)) {
         return std::nullopt;
      }

      return rcName.name();
   }

   [[nodiscard]] bool import_scene()
   {
      const auto& glbScene = m_glbFile.document->scenes[m_glbFile.document->scene];

      world::LevelNode rootNode("root");
      for (const u32 nodeID : glbScene.nodes) {
         auto& glbNode = m_glbFile.document->nodes[nodeID];

         if (!glbNode.mesh.has_value())
            continue;

         auto transform = Transform3D::identity();
         if (glbNode.matrix.has_value()) {
            transform = Transform3D::from_matrix(glbNode.matrix.value());
         }
         if (glbNode.rotation.has_value()) {
            transform.rotation = {glbNode.rotation->w, glbNode.rotation->x, glbNode.rotation->y, glbNode.rotation->z};
         }
         if (glbNode.translation.has_value()) {
            transform.translation = glbNode.translation.value();
         }
         if (glbNode.scale.has_value()) {
            transform.scale = glbNode.scale.value();
         }

         auto meshName = this->import_mesh(glbNode.name, *glbNode.mesh);
         if (!meshName.has_value()) {
            return false;
         }

         world::StaticMesh mesh;
         mesh.name = glbNode.name.value_or(fmt::format("static_mesh{}", nodeID));
         mesh.transform = transform;
         mesh.meshName = *meshName;

         rootNode.add_static_mesh(std::move(mesh));
      }

      world::Level level;
      level.add_node("root"_name, std::move(rootNode));

      if (!level.save_to_file(m_props.dstPath)) {
         fmt::print(stderr, "triglav-cli: Failed to save level file\n");
         return false;
      }

      return true;
   }

 private:
   LevelImportProps m_props;
   ProjectInfo m_projectInfo;
   gltf::GlbResource m_glbFile;
   std::map<u32, Name> m_importedMeshes;
};

bool import_level(const LevelImportProps& props)
{
   fmt::print(stderr, "triglav-cli: Importing level to {}\n", props.dstPath.string());

   auto projectInfo = load_active_project_info();
   if (!projectInfo.has_value()) {
      fmt::print(stderr, "triglav-cli: Failed to load project info\n");
      return false;
   }

   auto glbFile = gltf::open_glb_file(props.srcPath);
   if (!glbFile.has_value()) {
      fmt::print(stderr, "triglav-cli: Failed to load GLB file\n");
      return false;
   }

   LevelImporter importer(props, std::move(*projectInfo), std::move(*glbFile));
   if (!importer.import_scene()) {
      return false;
   }

   return true;
}

}// namespace triglav::tool::cli
