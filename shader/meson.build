shader_compiler = find_program('glslc')
compile_fragment_cmds = [shader_compiler, '-fshader-stage=frag', '@INPUT@', '-o', '@OUTPUT@']
compile_vertex_cmds = [shader_compiler, '-fshader-stage=vert', '@INPUT@', '-o', '@OUTPUT@']
compile_compute_cmds = [shader_compiler, '-fshader-stage=comp', '@INPUT@', '-o', '@OUTPUT@']
compile_raygen_cmds = [shader_compiler, '-fshader-stage=rgen', '--target-env=vulkan1.3', '@INPUT@', '-o', '@OUTPUT@']
compile_raychit_cmds = [shader_compiler, '-fshader-stage=rchit', '--target-env=vulkan1.3', '@INPUT@', '-o', '@OUTPUT@']
compile_raymiss_cmds = [shader_compiler, '-fshader-stage=rmiss', '--target-env=vulkan1.3', '@INPUT@', '-o', '@OUTPUT@']

shader_targets = []

subdir('ambient_occlusion')
subdir('bindless_geometry')
subdir('blur')
subdir('debug_lines')
subdir('ground')
subdir('particles')
subdir('pbr_normal_map')
subdir('pbr_parallax')
subdir('pbr_simple')
subdir('pbr_full')
subdir('post_processing')
subdir('ray_tracing')
subdir('shadow_map')
subdir('skybox')
subdir('rectangle')
subdir('shading')
subdir('sprite')
subdir('ssr')
subdir('text')

shaders = declare_dependency(sources: [shader_targets])
