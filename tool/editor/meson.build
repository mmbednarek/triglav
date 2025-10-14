editor_sources = files([
                           'src/Editor.cpp',
                           'src/Editor.hpp',
                           'src/Main.cpp',
                       ])

editor_deps = [
    renderer,
    launcher,
    geometry,
    desktop,
    threading,
    fmt,
    shaders,
    desktop_ui,
]

editor = executable('editor',
                    sources : editor_sources,
                    dependencies : editor_deps,
                    install : true,
                    install_dir : 'bin',
                    win_subsystem : 'windows',
                    link_whole : [desktop_main_lib],
)
