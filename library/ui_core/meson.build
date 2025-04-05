ui_core_sources = files([
  'include/triglav/ui_core/widget/AlignmentBox.hpp',
  'include/triglav/ui_core/widget/Button.hpp',
  'include/triglav/ui_core/widget/EmptySpace.hpp',
  'include/triglav/ui_core/widget/HideableWidget.hpp',
  'include/triglav/ui_core/widget/HorizontalLayout.hpp',
  'include/triglav/ui_core/widget/Image.hpp',
  'include/triglav/ui_core/widget/RectBox.hpp',
  'include/triglav/ui_core/widget/TextBox.hpp',
  'include/triglav/ui_core/widget/VerticalLayout.hpp',
  'include/triglav/ui_core/Context.hpp',
  'include/triglav/ui_core/IWidget.hpp',
  'include/triglav/ui_core/Primitives.hpp',
  'include/triglav/ui_core/UICore.hpp',
  'include/triglav/ui_core/Viewport.hpp',
  'src/widget/AlignmentBox.cpp',
  'src/widget/Button.cpp',
  'src/widget/EmptySpace.cpp',
  'src/widget/HideableWidget.cpp',
  'src/widget/HorizontalLayout.cpp',
  'src/widget/Image.cpp',
  'src/widget/RectBox.cpp',
  'src/widget/TextBox.cpp',
  'src/widget/VerticalLayout.cpp',
  'src/Context.cpp',
  'src/IWidget.cpp',
  'src/Viewport.cpp',
])

ui_core_deps = [glm, geometry, font, io, resource, core, render_core]
ui_core_incl = include_directories(['include', 'include/triglav/ui_core'])

ui_core_lib = static_library('ui_core',
  sources: ui_core_sources,
  dependencies: ui_core_deps,
  include_directories: ui_core_incl,
)

ui_core = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: [ui_core_lib],
  dependencies: ui_core_deps,
)
