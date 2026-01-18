project_sources = files([
                            'include/triglav/project/Name.hpp',
                            'include/triglav/project/PathManager.hpp',
                            'include/triglav/project/Project.hpp',
                            'include/triglav/project/ProjectManager.hpp',
                            'src/PathManager.cpp',
                            'src/Project.cpp',
                            'src/ProjectManager.cpp',
                        ])

project_deps = [core, json_util, meta, io, threading]
project_incl = include_directories(['include', 'include/triglav/project'])

project_lib = static_library('project',
                             sources : project_sources,
                             dependencies : project_deps,
                             include_directories : project_incl,
)

project = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : project_lib,
    dependencies : project_deps,
)
