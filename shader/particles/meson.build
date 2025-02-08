shader_targets += custom_target('shader_particles_compute',
                                input: 'update_position.slang',
                                output: 'compute.spv',
                                command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_particles_vertex',
                                input: 'render.slang',
                                output: 'vertex.spv',
                                command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-O3', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_particles_fragment',
                                input: 'render.slang',
                                output: 'fragment.spv',
                                command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6+spvImageQuery+spvSparseResidency', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@'],
)
