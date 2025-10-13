render_objects_sources = files([
                                   'include/triglav/render_objects/Material.hpp',
                                   'include/triglav/render_objects/Mesh.hpp',
                                   'include/triglav/render_objects/RenderObjects.hpp',
                                   'src/Material.cpp',
                                   'src/RenderObjects.cpp'
                               ])

render_objects_deps_priv = [glm, graphics_api, core, geometry, rapidyaml]
render_objects_deps_pub = [glm, graphics_api, core, geometry]
render_objects_incl = include_directories(['include', 'include/triglav/render_objects'])

render_objects_lib = static_library('render_objects',
                                    sources : render_objects_sources,
                                    dependencies : render_objects_deps_priv,
                                    include_directories : render_objects_incl,
)

render_objects = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : render_objects_lib,
    dependencies : render_objects_deps_pub,
)
