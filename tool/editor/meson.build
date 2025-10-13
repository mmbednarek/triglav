editor_sources = files([
                         'src/Main.cpp',
                     ])

editor_deps = [
    renderer,
    geometry,
    desktop,
    threading,
    fmt,
    shaders,
]

editor = executable('editor',
                  sources: editor_sources,
                  dependencies: editor_deps,
                  install: true,
                  install_dir: 'bin',
                  win_subsystem: 'windows',
                  link_whole: [desktop_main_lib],
)
