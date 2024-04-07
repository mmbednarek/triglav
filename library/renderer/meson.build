renderer_sources = files([
  'include/triglav/renderer/AmbientOcclusionRenderer.h',
  'include/triglav/renderer/Camera.h',
  'include/triglav/renderer/CameraBase.h',
  'include/triglav/renderer/DebugLinesRenderer.h',
  'include/triglav/renderer/GroundRenderer.h',
  'include/triglav/renderer/ModelRenderer.h',
  'include/triglav/renderer/OrthoCamera.h',
  'include/triglav/renderer/PostProcessingRenderer.h',
  'include/triglav/renderer/RectangleRenderer.h',
  'include/triglav/renderer/Renderer.h',
  'include/triglav/renderer/Scene.h',
  'include/triglav/renderer/ShadingRenderer.h',
  'include/triglav/renderer/ShadowMapRenderer.h',
  'include/triglav/renderer/SkyBox.h',
  'include/triglav/renderer/SpriteRenderer.h',
  'include/triglav/renderer/TextRenderer.h',
  'src/node/AmbientOcclusion.cpp',
  'src/node/ShadowMap.cpp',
  'src/node/Geometry.cpp',
  'src/node/PostProcessing.cpp',
  'src/node/Shading.cpp',
  'src/node/ShadowMap.cpp',
  'src/node/UserInterface.cpp',
  'src/AmbientOcclusionRenderer.cpp',
  'src/Camera.cpp',
  'src/CameraBase.cpp',
  'src/DebugLinesRenderer.cpp',
  'src/GroundRenderer.cpp',
  'src/ModelRenderer.cpp',
  'src/OrthoCamera.cpp',
  'src/PostProcessingRenderer.cpp',
  'src/RectangleRenderer.cpp',
  'src/Renderer.cpp',
  'src/Scene.cpp',
  'src/ShadingRenderer.cpp',
  'src/ShadowMapRenderer.cpp',
  'src/SkyBox.cpp',
  'src/SpriteRenderer.cpp',
  'src/TextRenderer.cpp',
])




renderer_deps = [glm, graphics_api, geometry, font, fonts, io, resource, render_core]
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
