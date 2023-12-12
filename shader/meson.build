shader_compiler = find_program('glslc')
compile_fragment_cmds = [shader_compiler, '-fshader-stage=frag', '@INPUT@', '-o', '@OUTPUT@']
compile_vertex_cmds = [shader_compiler, '-fshader-stage=vert', '@INPUT@', '-o', '@OUTPUT@']

shader_targets = []

shader_targets += custom_target('shader_example_fragment',
    input: 'example_fragment.glsl',
    output: '@BASENAME@.spv',
    command: compile_fragment_cmds,
)

shader_targets += custom_target('shader_example_vertex',
    input: 'example_vertex.glsl',
    output: '@BASENAME@.spv',
    command: compile_vertex_cmds,
)

subdir('skybox')

shaders = declare_dependency(sources: [shader_targets])
