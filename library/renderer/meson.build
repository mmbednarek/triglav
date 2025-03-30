renderer_sources = files([
  'include/triglav/renderer/stage/AmbientOcclusionStage.hpp',
  'include/triglav/renderer/stage/GBufferStage.hpp',
  'include/triglav/renderer/stage/IStage.hpp',
  'include/triglav/renderer/stage/PostProcessStage.hpp',
  'include/triglav/renderer/stage/RayTracingStage.hpp',
  'include/triglav/renderer/stage/ShadingStage.hpp',
  'include/triglav/renderer/stage/ShadowMapStage.hpp',
  'include/triglav/renderer/BindlessScene.hpp',
  'include/triglav/renderer/Camera.hpp',
  'include/triglav/renderer/CameraBase.hpp',
  'include/triglav/renderer/Config.hpp',
  'include/triglav/renderer/DebugLinesRenderer.hpp',
  'include/triglav/renderer/DebugWidget.hpp',
  'include/triglav/renderer/InfoDialog.hpp',
  'include/triglav/renderer/OcclusionCulling.hpp',
  'include/triglav/renderer/OrthoCamera.hpp',
  'include/triglav/renderer/RayTracingScene.hpp',
  'include/triglav/renderer/Renderer.hpp',
  'include/triglav/renderer/RenderingJob.hpp',
  'include/triglav/renderer/RenderSurface.hpp',
  'include/triglav/renderer/Scene.hpp',
  'include/triglav/renderer/StatisticManager.hpp',
  'include/triglav/renderer/UpdateUserInterfaceJob.hpp',
  'include/triglav/renderer/UpdateViewParamsJob.hpp',
  'include/triglav/renderer/Util.hpp',
  'src/stage/AmbientOcclusionStage.cpp',
  'src/stage/GBufferStage.cpp',
  'src/stage/PostProcessStage.cpp',
  'src/stage/RayTracingStage.cpp',
  'src/stage/ShadingStage.cpp',
  'src/stage/ShadowMapStage.cpp',
  'src/BindlessScene.cpp',
  'src/Camera.cpp',
  'src/CameraBase.cpp',
  'src/Config.cpp',
  'src/DebugLinesRenderer.cpp',
  'src/DebugWidget.cpp',
  'src/InfoDialog.cpp',
  'src/OcclusionCulling.cpp',
  'src/OrthoCamera.cpp',
  'src/RayTracingScene.cpp',
  'src/Renderer.cpp',
  'src/RenderingJob.cpp',
  'src/RenderSurface.cpp',
  'src/Scene.cpp',
  'src/StatisticManager.cpp',
  'src/UpdateUserInterfaceJob.cpp',
  'src/UpdateViewParamsJob.cpp',
  'src/Util.cpp',
])




renderer_deps = [glm, graphics_api, geometry, font, io, resource, render_core, ui_core, memory]
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
