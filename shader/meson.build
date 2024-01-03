shader_compiler = find_program('glslc')
compile_fragment_cmds = [shader_compiler, '-fshader-stage=frag', '@INPUT@', '-o', '@OUTPUT@']
compile_vertex_cmds = [shader_compiler, '-fshader-stage=vert', '@INPUT@', '-o', '@OUTPUT@']

shader_targets = []

subdir('model')
subdir('shadow_map')
subdir('skybox')
subdir('sprite')
subdir('text')

shaders = declare_dependency(sources: [shader_targets])
