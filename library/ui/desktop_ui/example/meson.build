desktop_ui_example_sources = files(
    'Main.cpp',
)

desktop_ui_example_deps = [render_core,
                           desktop,
                           desktop_ui,
                           shaders]

desktop_ui_example = executable('desktop_ui_example',
                                sources : desktop_ui_example_sources,
                                dependencies : desktop_ui_example_deps,
                                win_subsystem : 'windows',
                                link_whole : [desktop_main_lib],
)
