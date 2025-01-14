shader_targets += custom_target('shader_ground_vertex',
                                input : 'ground.slang',
                                output : 'vertex.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_ground_fragment',
                                input : 'ground.slang',
                                output : 'fragment.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-o', '@OUTPUT@'],
)
