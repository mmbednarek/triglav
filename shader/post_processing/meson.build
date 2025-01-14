shader_targets += custom_target('shader_post_processing_fragment',
                                input : 'post_processing.slang',
                                output : 'fragment.spv',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-o', '@OUTPUT@'],
)
