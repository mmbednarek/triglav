font_sources = files([
                         'include/triglav/font/Charset.hpp',
                         'include/triglav/font/FontManager.hpp',
                         'include/triglav/font/FreeTypeForwardDecl.hpp',
                         'include/triglav/font/Typeface.hpp',
                         'src/Charset.cpp',
                         'src/FontManager.cpp',
                         'src/Typeface.cpp',
                     ])

font_lib = static_library('font',
                          sources : font_sources,
                          dependencies : [freetype, io, threading],
                          include_directories : ['include/triglav/font'],
)

font = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : font_lib,
    dependencies: [io]
)
