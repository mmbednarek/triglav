io_sources = files([
  'src/File.cpp',
])

io_deps = [core]
io_incl = include_directories(['include', 'include/triglav/io'])

io_lib = static_library('io',
  sources: io_sources,
  dependencies: io_deps,
  include_directories: io_incl,
)

io = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: io_lib,
  dependencies: io_deps,
)
