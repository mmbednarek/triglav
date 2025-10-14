launcher_sources = files([
                             'include/triglav/launcher/Application.hpp',
                             'src/GraphicsDeviceInitStage.cpp',
                             'src/GraphicsDeviceInitStage.hpp',
                             'src/Launcher.cpp',
                             'src/LoadAllResourcesStage.cpp',
                             'src/LoadAllResourcesStage.hpp',
                             'src/LoadInitialResourcesStage.cpp',
                             'src/LoadInitialResourcesStage.hpp',
                             'src/Application.cpp',
                         ])

launcher_lib = static_library('launcher',
                              sources : launcher_sources,
                              dependencies : [core, io, resource, desktop, render_core],
                              include_directories : ['include/triglav/launcher'],
)

launcher = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : launcher_lib,
    dependencies : [io]
)
