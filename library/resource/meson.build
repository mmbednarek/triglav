resource_sources = files([
  'include/triglav/resource/Container.hpp',
  'include/triglav/resource/LevelLoader.h',
  'include/triglav/resource/LoadContext.h',
  'include/triglav/resource/Loader.hpp',
  'include/triglav/resource/MaterialLoader.h',
  'include/triglav/resource/ModelLoader.h',
  'include/triglav/resource/NameRegistry.h',
  'include/triglav/resource/PathManager.h',
  'include/triglav/resource/Resource.hpp',
  'include/triglav/resource/ResourceManager.h',
  'include/triglav/resource/ShaderLoader.h',
  'include/triglav/resource/TextureLoader.h',
  'include/triglav/resource/TypefaceLoader.h',
  'src/LevelLoader.cpp',
  'src/LoadContext.cpp',
  'src/MaterialLoader.cpp',
  'src/NameRegistry.cpp',
  'src/ResourceManager.cpp',
  'src/TextureLoader.cpp',
  'src/ShaderLoader.cpp',
  'src/ModelLoader.cpp',
  'src/TypefaceLoader.cpp',
  'src/PathManager.cpp',
])

resource_deps = [glm, graphics_api, core, geometry, render_core, font, rapidyaml, world, spdlog, threading]
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
