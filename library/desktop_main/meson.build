desktop_main_sources = files([])

if host_machine.system() == 'linux'
    subdir('platform/linux')
endif

desktop_main_lib = static_library('desktop_main',
                          sources : desktop_main_sources,
                          dependencies : [core, desktop]
)

desktop_main = declare_dependency(
    link_with : desktop_main_lib,
)
