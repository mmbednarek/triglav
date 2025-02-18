resource_sources = files([
  'include/triglav/resource/Container.hpp',
  'include/triglav/resource/LevelLoader.hpp',
  'include/triglav/resource/LoadContext.hpp',
  'include/triglav/resource/Loader.hpp',
  'include/triglav/resource/MaterialLoader.hpp',
  'include/triglav/resource/MeshLoader.hpp',
  'include/triglav/resource/NameRegistry.hpp',
  'include/triglav/resource/PathManager.hpp',
  'include/triglav/resource/Resource.hpp',
  'include/triglav/resource/ResourceManager.hpp',
  'include/triglav/resource/ShaderLoader.hpp',
  'include/triglav/resource/TextureLoader.hpp',
  'include/triglav/resource/TypefaceLoader.hpp',
  'src/LevelLoader.cpp',
  'src/LoadContext.cpp',
  'src/MaterialLoader.cpp',
  'src/NameRegistry.cpp',
  'src/ResourceManager.cpp',
  'src/TextureLoader.cpp',
  'src/ShaderLoader.cpp',
  'src/MeshLoader.cpp',
  'src/TypefaceLoader.cpp',
  'src/PathManager.cpp',
])

resource_deps = [glm, graphics_api, core, geometry, font, rapidyaml, world, spdlog, threading, render_objects, event, gltf, asset, tg_ktx]
resource_incl = include_directories(['include', 'include/triglav/resource'])

resource_lib = static_library('resource',
                              sources : resource_sources,
                              dependencies : resource_deps,
                              include_directories : resource_incl,
)

resource = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : resource_lib,
    dependencies : resource_deps,
)

subdir('test')
