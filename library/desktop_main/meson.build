desktop_main_sources = files([])

if host_machine.system() == 'linux'
    subdir('platform/linux')
elif host_machine.system() == 'windows'
    subdir('platform/windows')
endif

desktop_main_lib = static_library('desktop_main',
                          sources : desktop_main_sources,
                          dependencies : [core, desktop, spdlog],
)

desktop_main = declare_dependency(
    link_with : desktop_main_lib,
)
