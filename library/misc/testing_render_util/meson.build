testing_render_util_sources = files([
                           'include/triglav/testing_render_util/RenderSupport.hpp',
                           'src/RenderSupport.cpp',
                       ])

testing_render_util_lib = static_library('testing_render_util',
                            sources : testing_render_util_sources,
                            dependencies : [testing_core, io],
                            include_directories : ['include/triglav/testing_render_util'],
)

testing_render_util = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : testing_render_util_lib,
    dependencies : [testing_core, io]
)
