#include "Engine.hpp"

#include "triglav/io/File.hpp"
#include "triglav/project/PathManager.hpp"
#include "triglav/project/ProjectManager.hpp"

#include <ryml.hpp>

namespace triglav::engine {

using namespace name_literals;

namespace {

Vector3 parse_vector3(const ryml::ConstNodeRef node)
{
   auto x = node["x"].val();
   auto y = node["y"].val();
   auto z = node["z"].val();

   return Vector3{
      std::stof(std::string{x.data(), x.size()}),
      std::stof(std::string{y.data(), y.size()}),
      std::stof(std::string{z.data(), z.size()}),
   };
}

Vector4 parse_vector4(const ryml::ConstNodeRef node)
{
   auto x = node["x"].val();
   auto y = node["y"].val();
   auto z = node["z"].val();
   auto w = node["w"].val();

   return Vector4{
      std::stof(std::string{x.data(), x.size()}),
      std::stof(std::string{y.data(), y.size()}),
      std::stof(std::string{z.data(), z.size()}),
      std::stof(std::string{w.data(), w.size()}),
   };
}

template<ResourceType CResType>
TypedName<CResType> to_typed_name(const ryml::csubstr str)
{
   if (std::ranges::all_of(str, [](const char c) { return std::isdigit(c); })) {
      return TypedName<CResType>{std::stoull(std::string{str.data(), str.size()})};
   }

   return TypedName<CResType>{name_from_path({str.data(), str.size()})};
}

Transform3D parse_transformation(const ryml::ConstNodeRef node)
{
   const auto translation = node["translation"];
   const auto rotation = node["rotation"];
   const auto scale = node["scale"];

   const auto rotation_vec4 = parse_vector4(rotation);

   return Transform3D{
      .rotation = glm::quat{rotation_vec4.w, rotation_vec4.x, rotation_vec4.y, rotation_vec4.z},
      .scale = parse_vector3(scale),
      .translation = parse_vector3(translation),
   };
}

world::StaticMesh parse_static_mesh(const ryml::ConstNodeRef node)
{
   const auto mesh_name = node["mesh"].val();
   const auto name = node["name"].val();
   std::optional<ArmatureName> armature_name{};
   if (node.has_child("armature")) {
      const auto armature = node["armature"].val();
      armature_name.emplace(name_from_path({armature.data(), armature.size()}));
   }

   return world::StaticMesh{
      .mesh_name = to_typed_name<ResourceType::Mesh>(mesh_name),
      .name = {name.data(), name.size()},
      .transform = parse_transformation(node["transform"]),
      .armature_name = armature_name,
   };
}

}// namespace

void Engine::initialize(graphics_api::Device& device)
{
   m_status.store(EngineStatus::Initializing);
   m_resource_manager = std::make_unique<resource::ResourceManager>(device, m_font_manger);
   TG_CONNECT_OPT(*m_resource_manager, OnLoadedAssets, on_loaded_assets);
   log_info("Initialising engine");

   m_resource_manager->load_asset_list(project::PathManager::the().translate_path("engine/index.yaml"_rc));
}

void Engine::destroy()
{
   m_levels.clear();
   m_pending_level.reset();
   m_resource_manager.reset();
   m_status.store(EngineStatus::Uninitialized);
}

void Engine::load_level(const LevelName level_name)
{
   assert(m_resource_manager != nullptr);

   log_info("Loading level {}", ResourcePathMap::the().resolve(level_name));

   const auto level_path = project::PathManager::the().translate_path(level_name);
   auto file = io::read_whole_file(level_path);
   assert(not file.empty());

   auto tree = ryml::parse_in_place(c4::substr{const_cast<char*>(level_path.string().data()), level_path.string().size()},
                                    c4::substr{file.data(), file.size()});

   std::vector<ResourceName> resource_list;

   if (project::this_project() != "triglav_editor"_name) {
      // If not the editor, reset all levels
      m_levels.clear();
   }
   m_pending_level_name = level_name;
   m_pending_level = std::make_unique<world::Level>();
   m_pending_level->init_entities();

   auto nodes = tree["nodes"];
   for (const auto& node : nodes) {
      auto name = node["name"].val();

      world::LevelNode level_node({name.data(), name.size()});

      auto items = node["items"];
      for (const auto item : items) {
         auto type_val = item["type"].val();
         if (type_val == "static_mesh") {
            auto mesh = parse_static_mesh(item);
            resource_list.emplace_back(mesh.mesh_name);
            if (mesh.armature_name.has_value()) {
               resource_list.emplace_back(*mesh.armature_name);
            }
            level_node.add_static_mesh(std::move(mesh));
         }
      }

      m_pending_level->add_node(make_name_id(std::string_view{name.data(), name.size()}), std::move(level_node));
   }

   m_status.store(EngineStatus::LoadingLevel);
   m_resource_manager->load_assets(resource_list);
}

void Engine::on_loaded_assets()
{
   const auto status = m_status.load();
   switch (status) {
   case EngineStatus::Initializing: {
      m_status.store(EngineStatus::Ready);
      event_OnEngineReady.publish();
      return;
   }
   case EngineStatus::LoadingLevel: {
      log_info("Level ready");
      m_status.store(EngineStatus::LevelLoaded);
      m_levels.emplace(m_pending_level_name, std::move(m_pending_level));
      m_current_level_name = m_pending_level_name;
      m_current_level = m_levels.at(m_current_level_name).get();
      m_pending_level_name = {};
      event_OnLevelLoaded.publish();
      return;
   }
   default:
      break;
   }
}

resource::ResourceManager& Engine::resource_manager() const
{
   assert(m_resource_manager != nullptr);
   return *m_resource_manager;
}

void Engine::on_begin_frame(float /*delta_time*/) const
{
   if (m_current_level != nullptr) {
      m_current_level->flush();
   }
}

void Engine::unload_level(const LevelName name)
{
   if (m_current_level_name == name) {
      m_current_level_name = {};
      m_current_level = nullptr;
   }
   m_levels.erase(name);
}

void Engine::set_active_level(const LevelName name)
{
   m_current_level_name = name;
   m_current_level = m_levels.at(name).get();
}

world::Level* Engine::current_level() const
{
   assert(m_current_level == nullptr || m_current_level == m_levels.at(m_current_level_name).get());
   return m_current_level;
}

Engine& Engine::the()
{
   static Engine engine;
   return engine;
}

}// namespace triglav::engine