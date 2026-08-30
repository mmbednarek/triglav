engine_sources = files([
                             'include/triglav/engine/Engine.hpp',
                             'src/Engine.cpp',
                         ])

engine_lib = static_library('engine',
                              sources : engine_sources,
                              dependencies : [core, io, resource, project, font],
                              include_directories : ['include/triglav/engine'],
)

engine = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : engine_lib,
    dependencies : [io, resource]
)
