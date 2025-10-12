desktop_ui_sources = files([
                          'include/triglav/desktop_ui/Button.hpp',
                          'include/triglav/desktop_ui/CheckBox.hpp',
                          'include/triglav/desktop_ui/ContextMenu.hpp',
                          'include/triglav/desktop_ui/DesktopUI.hpp',
                          'include/triglav/desktop_ui/Dialog.hpp',
                          'include/triglav/desktop_ui/DialogManager.hpp',
                          'include/triglav/desktop_ui/DropDownMenu.hpp',
                          'include/triglav/desktop_ui/MenuBar.hpp',
                          'include/triglav/desktop_ui/MenuController.hpp',
                          'include/triglav/desktop_ui/MenuList.hpp',
                          'include/triglav/desktop_ui/Splitter.hpp',
                          'include/triglav/desktop_ui/TextInput.hpp',
                          'include/triglav/desktop_ui/TreeController.hpp',
                          'include/triglav/desktop_ui/TreeView.hpp',
                          'src/Button.cpp',
                          'src/CheckBox.cpp',
                          'src/ContextMenu.cpp',
                          'src/DesktopUI.cpp',
                          'src/Dialog.cpp',
                          'src/DialogManager.cpp',
                          'src/DropDownMenu.cpp',
                          'src/MenuBar.cpp',
                          'src/MenuController.cpp',
                          'src/MenuList.cpp',
                          'src/Splitter.cpp',
                          'src/TextInput.cpp',
                          'src/TreeController.cpp',
                          'src/TreeView.cpp',
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