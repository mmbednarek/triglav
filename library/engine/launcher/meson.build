launcher_sources = files([
                             'include/triglav/launcher/Launcher.hpp',
                             'src/Launcher.cpp',
                         ])

launcher_lib = static_library('launcher',
                              sources : launcher_sources,
                              dependencies : [core, io, resource],
                              include_directories : ['include/triglav/launcher'],
)

launcher = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : launcher_lib,
    dependencies : [io]
)
