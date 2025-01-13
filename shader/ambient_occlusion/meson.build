shader_targets += custom_target('shader_ambient_occlusion_fragment',
                                input : 'ambient_occlusion.slang',
                                output : 'fragment.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@'],
)
