desktop_sources = files([])

if host_machine.system() == 'linux'
    if get_option('wayland').enabled()
        subdir('platform/wayland')
    else
        subdir('platform/x11')
    endif
endif

desktop_lib = static_library('desktop',
                          sources : desktop_sources,
                          dependencies : [wayland_client, xlib],
                          include_directories : ['include', 'include/triglav/desktop'],
)

desktop = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : desktop_lib,
)
