graphics_api_sources = files([
  'src/Buffer.cpp',
  'src/CommandList.cpp',
  'src/DescriptorGroup.cpp',
  'src/Device.cpp',
  'src/PipelineBuilder.cpp',
  'src/Pipeline.cpp',
  'src/PlatformSurface.cpp',
  'src/RenderPass.cpp',
  'src/Shader.cpp',
  'src/Synchronization.cpp',
  'src/Texture.cpp',
  'src/vulkan/Extensions.cpp',
  'src/vulkan/Util.cpp',
  'src/vulkan/Util.h',
])

graphics_api_deps = [vulkan, wayland_client]
graphics_api_incl = include_directories(['include', 'include/graphics_api'])

graphics_api_lib = static_library('graphics_api',
  sources: graphics_api_sources,
  dependencies: graphics_api_deps,
  include_directories: graphics_api_incl,
)

graphics_api = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: graphics_api_lib,
)
