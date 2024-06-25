font_sources = files([
                         'include/triglav/font/Charset.h',
                         'include/triglav/font/FontManager.h',
                         'include/triglav/font/FreeTypeForwardDecl.h',
                         'include/triglav/font/Typeface.h',
                         'include/triglav/font/Utf8StringView.h',
                         'src/Charset.cpp',
                         'src/FontManager.cpp',
                         'src/Typeface.cpp',
                         'src/Utf8StringView.cpp',
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
