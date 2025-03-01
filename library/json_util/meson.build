json_util_sources = files([
  'include/triglav/json_util/JsonUtil.hpp',
  'src/JsonUtil.cpp',
])

json_util_deps = [core, io, rapidjson]
json_util_incl = include_directories(['include', 'include/triglav/json_util'])

json_util_lib = static_library('json_util',
  sources: json_util_sources,
  dependencies: json_util_deps,
  include_directories: json_util_incl,
)

json_util = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: [json_util_lib],
  dependencies: json_util_deps,
)
