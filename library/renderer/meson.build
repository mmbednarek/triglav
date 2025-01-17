renderer_sources = files([
  'include/triglav/renderer/AmbientOcclusionRenderer.hpp',
  'include/triglav/renderer/BindlessScene.hpp',
  'include/triglav/renderer/Camera.hpp',
  'include/triglav/renderer/CameraBase.hpp',
  'include/triglav/renderer/DebugLinesRenderer.hpp',
  'include/triglav/renderer/GlyphCache.hpp',
  'include/triglav/renderer/GroundRenderer.hpp',
  'include/triglav/renderer/InfoDialog.hpp',
  'include/triglav/renderer/MaterialManager.hpp',
  'include/triglav/renderer/OrthoCamera.hpp',
  'include/triglav/renderer/PostProcessingRenderer.hpp',
  'include/triglav/renderer/RayTracingScene.hpp',
  'include/triglav/renderer/RectangleRenderer.hpp',
  'include/triglav/renderer/Renderer.hpp',
  'include/triglav/renderer/Scene.hpp',
  'include/triglav/renderer/ShadingRenderer.hpp',
  'include/triglav/renderer/SkyBox.hpp',
  'include/triglav/renderer/SpriteRenderer.hpp',
  'include/triglav/renderer/StatisticManager.hpp',
  'include/triglav/renderer/TextRenderer.hpp',
  'include/triglav/renderer/TextureHelper.hpp',
  'src/node/AmbientOcclusion.cpp',
  'src/node/AmbientOcclusion.hpp',
  'src/node/BindlessGeometry.cpp',
  'src/node/BindlessGeometry.hpp',
  'src/node/Blur.cpp',
  'src/node/Blur.hpp',
  'src/node/Downsample.cpp',
  'src/node/Downsample.hpp',
  'src/node/Geometry.cpp',
  'src/node/Geometry.hpp',
  'src/node/Particles.cpp',
  'src/node/Particles.hpp',
  'src/node/PostProcessing.cpp',
  'src/node/PostProcessing.hpp',
  'src/node/ProcessGlyphs.cpp',
  'src/node/ProcessGlyphs.hpp',
  'src/node/RayTracedImage.cpp',
  'src/node/RayTracedImage.hpp',
  'src/node/Shading.cpp',
  'src/node/Shading.hpp',
  'src/node/ShadowMap.cpp',
  'src/node/ShadowMap.hpp',
  'src/node/SyncBuffers.cpp',
  'src/node/SyncBuffers.hpp',
  'src/node/UserInterface.cpp',
  'src/node/UserInterface.hpp',
  'src/AmbientOcclusionRenderer.cpp',
  'src/BindlessScene.cpp',
  'src/Camera.cpp',
  'src/CameraBase.cpp',
  'src/DebugLinesRenderer.cpp',
  'src/GlyphCache.cpp',
  'src/GroundRenderer.cpp',
  'src/InfoDialog.cpp',
  'src/MaterialManager.cpp',
  'src/OrthoCamera.cpp',
  'src/PostProcessingRenderer.cpp',
  'src/RayTracingScene.cpp',
  'src/RectangleRenderer.cpp',
  'src/Renderer.cpp',
  'src/Scene.cpp',
  'src/ShadingRenderer.cpp',
  'src/StatisticManager.cpp',
  'src/SkyBox.cpp',
  'src/SpriteRenderer.cpp',
  'src/TextRenderer.cpp',
  'src/TextureHelper.cpp',
])




renderer_deps = [glm, graphics_api, geometry, font, io, resource, render_core, ui_core]
renderer_incl = include_directories(['include', 'include/triglav/renderer'])

renderer_lib = static_library('renderer',
  sources: renderer_sources,
  dependencies: renderer_deps,
  include_directories: renderer_incl,
)

renderer = declare_dependency(
  include_directories: include_directories(['include']),
  link_with: [renderer_lib],
  dependencies: renderer_deps,
)
