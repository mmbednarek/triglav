desktop_ui_sources = files([
                          'include/triglav/desktop_ui/Button.hpp',
                          'include/triglav/desktop_ui/DesktopUI.hpp',
                          'include/triglav/desktop_ui/Dialog.hpp',
                          'src/Button.cpp',
                          'src/DesktopUI.cpp',
                          'src/Dialog.cpp',
                      ])

desktop_ui_deps = [core, threading, ui_core, renderer]
desktop_ui_incl = include_directories(['include', 'include/triglav/desktop_ui'])

desktop_ui_lib = static_library('desktop_ui',
                           sources : desktop_ui_sources,
                           dependencies : desktop_ui_deps,
                           include_directories : desktop_ui_incl,
)

desktop_ui = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : desktop_ui_lib,
    dependencies : desktop_ui_deps,
)

subdir('example')