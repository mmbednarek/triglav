shader_targets += custom_target('shader_text_compute',
                                input: 'compute.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_compute_cmds,
)

shader_targets += custom_target('shader_text_vertex',
                                input: 'vertex.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_vertex_cmds,
)

shader_targets += custom_target('shader_text_fragment',
                                input: 'fragment.glsl',
                                output: '@BASENAME@.spv',
                                command: compile_fragment_cmds,
)

shader_targets += custom_target('text_generate_geometry_cs',
    input: 'generate_geometry.slang',
    output: '@BASENAME@.spv',
    depend_files: ['text_processing.slang'],
    command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('text_render_vs',
                                input: 'render.slang',
                                output: '@BASENAME@_vs.spv',
                                depend_files: ['text_processing.slang'],
                                command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'vs_main', '-stage', 'vertex', '-O3', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('text_render_fs',
                                input: 'render.slang',
                                output: '@BASENAME@_fs.spv',
                                depend_files: ['text_processing.slang'],
                                command: ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'fs_main', '-stage', 'fragment', '-O3', '-o', '@OUTPUT@'],
)
