#include "Project.hpp"
#include "Name.hpp"

#include "triglav/io/CommandLine.hpp"
#include "triglav/io/File.hpp"
#include "triglav/json_util/Deserialize.hpp"

namespace triglav::project {

std::string ProjectMetadata::default_import_path(const ResourceType res_type, const std::string_view basename) const
{
   std::string result;
   switch (res_type) {
   case ResourceType::Texture:
      result = this->import_settings.texture_path;
      break;
   case ResourceType::Mesh:
      result = this->import_settings.mesh_path;
      break;
   case ResourceType::Level:
      result = this->import_settings.level_path;
      break;
   case ResourceType::Material:
      result = this->import_settings.material_path;
      break;
   default:
      return "";
   }

   auto index = result.find("{basename}");
   while (index != std::string::npos) {
      result.replace(index, 10, basename);
      index = result.find("{basename}");
   }
   return result;
}

}// namespace triglav::project

#define TG_NAMESPACE(NS, TYPE_NAME) NS(NS(triglav, project), TYPE_NAME)

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectType)
TG_META_ENUM_BEGIN
TG_META_ENUM_VALUE(Engine)
TG_META_ENUM_VALUE(Tool)
TG_META_ENUM_VALUE(Game)
TG_META_ENUM_VALUE(Test)
TG_META_ENUM_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ResourcePathMapping)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(engine_path, std::string)
TG_META_PROPERTY(system_path, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ImportSettings)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(texture_path, std::string)
TG_META_PROPERTY(mesh_path, std::string)
TG_META_PROPERTY(level_path, std::string)
TG_META_PROPERTY(material_path, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectMetadata)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(name, std::string)
TG_META_PROPERTY(identifier, std::string)
TG_META_PROPERTY(type, triglav::project::ProjectType)
TG_META_PROPERTY(engine, std::string)
TG_META_ARRAY_PROPERTY(resource_mapping, triglav::project::ResourcePathMapping)
TG_META_PROPERTY(import_settings, triglav::project::ImportSettings)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectInfo)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(name, std::string)
TG_META_PROPERTY(path, std::string)
TG_META_CLASS_END
#undef TG_TYPE

#define TG_TYPE(NS) TG_NAMESPACE(NS, ProjectConfig)
TG_META_CLASS_BEGIN
TG_META_PROPERTY(active_project, std::string)
TG_META_ARRAY_PROPERTY(projects, triglav::project::ProjectInfo)
TG_META_CLASS_END
#undef TG_TYPE

#undef TG_NAMESPACE
