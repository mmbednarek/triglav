shader_targets += custom_target('shader_shading_vertex',
                                input : 'shading.slang',
                                output : 'vertex.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_shading_fragment',
                                input : 'shading.slang',
                                output : 'fragment.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-o', '@OUTPUT@'],
)
