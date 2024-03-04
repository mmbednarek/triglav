resource_sources = files([
                             'src/LevelLoader.cpp',
                             'src/MaterialLoader.cpp',
                             'src/ResourceManager.cpp',
                             'src/TextureLoader.cpp',
                             'src/ShaderLoader.cpp',
                             'src/StaticResources.cpp',
                             'src/ModelLoader.cpp',
                             'src/TypefaceLoader.cpp',
                         ])

resource_deps = [glm, graphics_api, core, geometry, render_core, font, fonts, rapidyaml, world, spdlog]
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