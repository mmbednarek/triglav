graphics_api_sources = files([
  'src/Buffer.cpp',
  'src/CommandList.cpp',
  'src/DepthRenderTarget.cpp',
  'src/DescriptorArray.cpp',
  'src/DescriptorPool.cpp',
  'src/DescriptorView.cpp',
  'src/DescriptorWriter.cpp',
  'src/Device.cpp',
  'src/Framebuffer.cpp',
  'src/PipelineBuilder.cpp',
  'src/Pipeline.cpp',
  'src/PlatformSurface.cpp',
  'src/RenderPass.cpp',
  'src/Sampler.cpp',
  'src/Shader.cpp',
  'src/Swapchain.cpp',
  'src/Synchronization.cpp',
  'src/Texture.cpp',
  'src/vulkan/Extensions.cpp',
  'src/vulkan/Util.cpp',
  'src/vulkan/Util.h',
])

graphics_api_lib = static_library('graphics_api',
  sources: graphics_api_sources,
  dependencies: [vulkan, wayland_client],
  include_directories: ['include/graphics_api'],
)

graphics_api = declare_dependency(
  include_directories: ['include'],
  link_with: graphics_api_lib,
)
