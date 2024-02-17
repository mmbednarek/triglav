#include "ResourceManager.h"

#include "TextureLoader.h"
#include "ShaderLoader.h"

namespace triglav::resource {

void ResourceManager::load_asset(const Name assetName, const std::string_view path)
{
   switch (assetName.type()) {
   case ResourceType::Texture: this->load_resource<ResourceType::Texture>(assetName, path); break;
   case ResourceType::Mesh: this->load_resource<ResourceType::Mesh>(assetName, path); break;
   case ResourceType::FragmentShader:
      this->load_resource<ResourceType::FragmentShader>(assetName, path);
      break;
   case ResourceType::VertexShader: this->load_resource<ResourceType::VertexShader>(assetName, path); break;
   case ResourceType::Material: this->load_resource<ResourceType::Material>(assetName, path); break;
   case ResourceType::Model: this->load_resource<ResourceType::Model>(assetName, path); break;
   case ResourceType::ResourceTypeFace:
      this->load_resource<ResourceType::ResourceTypeFace>(assetName, path);
      break;
   case ResourceType::Unknown: break;
   }
}

}// namespace triglav::resource