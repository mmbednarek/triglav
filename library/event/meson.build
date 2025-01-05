event_sources = files([
                          'include/triglav/event/Delegate.hpp',
                          'include/triglav/event/Entt.hpp',
                          'src/Event.cpp',
                      ])

event_deps = [core]
event_incl = include_directories(['include', 'include/triglav/event'])

event_lib = static_library('event',
                           sources : event_sources,
                           dependencies : event_deps,
                           include_directories : event_incl,
)

event = declare_dependency(
    include_directories : include_directories(['include']),
    link_with : event_lib,
    dependencies : event_deps,
)
