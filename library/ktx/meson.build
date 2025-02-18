tg_ktx_sources = files([
                           'include/triglav/ktx/Texture.hpp',
                           'include/triglav/ktx/ForwardDecl.h',
                           'src/Texture.cpp',
                           'src/Vulkan.cpp',
                       ])

tg_ktx_lib = static_library('tg_ktx',
                            sources : tg_ktx_sources,
                            dependencies : [core, io, ktx],
                            include_directories : ['include/triglav/ktx'],
)

tg_ktx = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : tg_ktx_lib,
    dependencies : [io]
)
