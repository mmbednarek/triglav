io_sources = files([
  'include/triglav/io/BufferedReader.hpp',
  'include/triglav/io/BufferWriter.hpp',
  'include/triglav/io/CommandLine.hpp',
  'include/triglav/io/Deserializer.hpp',
  'include/triglav/io/DisplacedStream.hpp',
  'include/triglav/io/DynamicWriter.hpp',
  'include/triglav/io/DynLibrary.hpp',
  'include/triglav/io/File.hpp',
  'include/triglav/io/LimitedReader.hpp',
  'include/triglav/io/Path.hpp',
  'include/triglav/io/Result.hpp',
  'include/triglav/io/Serializer.hpp',
  'include/triglav/io/Stream.hpp',
  'include/triglav/io/StringReader.hpp',
  'src/BufferWriter.cpp',
  'src/BufferedReader.cpp',
  'src/CommandLine.cpp',
  'src/Deserializer.cpp',
  'src/DisplacedStream.cpp',
  'src/DynamicWriter.cpp',
  'src/DynLibrary.cpp',
  'src/File.cpp',
  'src/LimitedReader.cpp',
  'src/Path.cpp',
  'src/Serializer.cpp',
  'src/StringReader.cpp',
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

subdir('test')
