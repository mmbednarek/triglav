world_sources = files([
                          'include/triglav/world/Level.hpp',
                          'include/triglav/world/LevelNode.hpp',
                          'src/Level.cpp',
                          'src/LevelNode.cpp',
                      ])

world_deps = [core]
world_incl = include_directories(['include', 'include/triglav/world'])

world_lib = static_library('world',
                           sources : world_sources,
                           dependencies : world_deps,
                           include_directories : world_incl,
)

world = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : world_lib,
    dependencies : world_deps,
)
