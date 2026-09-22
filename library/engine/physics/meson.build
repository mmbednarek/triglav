physics_sources = files([
# !GENERATED
                            'include/triglav/physics/PhysicsSystem.hpp',
                            'src/PhysicsSystem.cpp',
# !END
                        ])

physics_deps_priv = [core, io, meta, geometry, world, engine]
physics_deps_pub = [core, geometry, world]
physics_incl = include_directories(['include', 'include/triglav/physics'])

physics_lib = static_library('physics',
                             sources : physics_sources,
                             dependencies : physics_deps_priv,
                             include_directories : physics_incl,
)

physics = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : physics_lib,
    dependencies : physics_deps_pub,
)

