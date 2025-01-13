shader_targets += custom_target('shader_skybox_vertex',
    input: 'skybox.slang',
    output: 'vertex.spv',
    command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-O3', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_skybox_fragment',
    input: 'skybox.slang',
    output: 'fragment.spv',
    command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@'],
)
