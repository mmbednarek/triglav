editor_sources = files([
                           'src/Editor.cpp',
                           'src/Editor.hpp',
                           'src/LevelEditor.cpp',
                           'src/LevelEditor.hpp',
                           'src/LevelViewport.hpp',
                           'src/LevelViewport.cpp',
                           'src/Main.cpp',
                           'src/ProjectExplorer.cpp',
                           'src/ProjectExplorer.hpp',
                           'src/RenderViewport.cpp',
                           'src/RenderViewport.hpp',
                           'src/RootWidget.cpp',
                           'src/RootWidget.hpp',
                           'src/RootWindow.cpp',
                           'src/RootWindow.hpp',
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
