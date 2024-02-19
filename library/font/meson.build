font_sources = files([
                         'src/FontManager.cpp',
                         'src/Typeface.cpp',
                     ])

font_lib = static_library('font',
                          sources : font_sources,
                          dependencies : [freetype],
                          include_directories : ['include/triglav/font'],
)

font = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : font_lib,
)
