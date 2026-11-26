asset_sources = files([
                         'include/triglav/asset/Asset.hpp',
                         'include/triglav/asset/ShaderFormat.hpp',
                         'src/Asset.cpp',
                         'src/ShaderFormat.cpp',
                     ])

asset_lib = static_library('asset',
                          sources : asset_sources,
                          dependencies : [core, io, geometry, tg_ktx, meta, json_util],
                          include_directories : ['include/triglav/asset'],
)

asset = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : asset_lib,
    dependencies: [io, meta, geometry]
)
