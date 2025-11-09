desktop_sources = files([])

desktop_deps = [vulkan, core, event]

if host_machine.system() == 'linux'
    desktop_deps += wayland_client
    desktop_deps += xlib
    desktop_deps += xkb_common
    subdir('platform/linux')
elif host_machine.system() == 'windows'
    subdir('platform/windows')
endif

desktop_lib = static_library('desktop',
                          sources : desktop_sources,
                          dependencies : desktop_deps,
                          include_directories : ['include', 'include/triglav/desktop'],
)

desktop = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : desktop_lib,
    dependencies: [core, event],
)
