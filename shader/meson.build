shader_compiler = find_program('glslc')
compile_fragment_cmds = [shader_compiler, '-fshader-stage=frag', '@INPUT@', '-o', '@OUTPUT@']
compile_vertex_cmds = [shader_compiler, '-fshader-stage=vert', '@INPUT@', '-o', '@OUTPUT@']

shader_targets = []

subdir('model')
subdir('post_processing')
subdir('shadow_map')
subdir('skybox')
subdir('rectangle')
subdir('sprite')
subdir('text')
subdir('debug_lines')

shaders = declare_dependency(sources: [shader_targets])
