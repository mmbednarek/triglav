asset_sources = files([
                         'include/triglav/asset/Asset.hpp',
                         'src/Asset.cpp',
                     ])

asset_lib = static_library('asset',
                          sources : asset_sources,
                          dependencies : [core, io, geometry],
                          include_directories : ['include/triglav/asset'],
)

asset = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : asset_lib,
    dependencies: [io]
)
