io_sources = files([
  'include/triglav/io/BufferedReader.h',
  'include/triglav/io/BufferWriter.h',
  'include/triglav/io/CommandLine.h',
  'include/triglav/io/File.h',
  'include/triglav/io/Path.h',
  'include/triglav/io/Result.h',
  'include/triglav/io/Serializer.h',
  'include/triglav/io/Stream.h',
  'src/BufferWriter.cpp',
  'src/BufferedReader.cpp',
  'src/CommandLine.cpp',
  'src/File.cpp',
  'src/Path.cpp',
  'src/Serializer.cpp',
])

if host_machine.system() == 'linux'
    subdir('platform/linux')
elif host_machine.system() == 'windows'
    subdir('platform/windows')
endif

io_deps = [core, glm, fmt]
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
