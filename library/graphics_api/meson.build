graphics_api_sources = files([
                                 'src/Buffer.cpp',
                                 'src/CommandBatch.cpp',
                                 'src/CommandList.cpp',
                                 'src/DescriptorArray.cpp',
                                 'src/DescriptorPool.cpp',
                                 'src/DescriptorView.cpp',
                                 'src/DescriptorWriter.cpp',
                                 'src/Device.cpp',
                                 'src/Framebuffer.cpp',
                                 'src/Pipeline.cpp',
                                 'src/PipelineBuilder.cpp',
                                 'src/QueueManager.cpp',
                                 'src/Sampler.cpp',
                                 'src/Shader.cpp',
                                 'src/Swapchain.cpp',
                                 'src/Synchronization.cpp',
                                 'src/Texture.cpp',
                                 'src/TextureRenderTarget.cpp',
                                 'src/vulkan/Extensions.cpp',
                                 'src/vulkan/Util.cpp',
                                 'src/vulkan/Util.h',
                             ])

graphics_api_lib = static_library('graphics_api',
                                  sources : graphics_api_sources,
                                  dependencies : [vulkan, wayland_client, desktop, core],
                                  include_directories : ['include/triglav/graphics_api'],
)

graphics_api = declare_dependency(
    include_directories : ['include'],
    link_with : graphics_api_lib,
    dependencies : [desktop, core],
)
