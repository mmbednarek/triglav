world_sources = files([
# !GENERATED
        'include/triglav/world/ComponentManager.hpp',
        'include/triglav/world/ComponentStorage.hpp',
        'include/triglav/world/EntityStorage.hpp',
        'include/triglav/world/HierarchyTree.hpp',
        'include/triglav/world/Level.hpp',
        'include/triglav/world/LevelNode.hpp',
        'include/triglav/world/World.hpp',
        'src/ComponentManager.cpp',
        'src/ComponentStorage.cpp',
        'src/CoreComponents.cpp',
        'src/EntityStorage.cpp',
        'src/HierarchyTree.cpp',
        'src/Level.cpp',
        'src/LevelNode.cpp',
# !END
                      ])

world_deps_priv = [core, rapidyaml, io, meta, memory]
world_deps_pub = [core, meta, memory]
world_incl = include_directories(['include', 'include/triglav/world'])

world_lib = static_library('world',
                           sources : world_sources,
                           dependencies : world_deps_priv,
                           include_directories : world_incl,
)

world = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : world_lib,
    dependencies : world_deps_pub,
)


subdir('test')