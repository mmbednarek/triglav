shader_targets += custom_target('shader_ray_tracing_gen',
                                input : ['ray_generation.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rgenshader',
                                command : ['slangc', '-O0', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT0@', '-entry', 'rgen_main', '-stage', 'raygeneration', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_ray_tracing_chit',
                                input : 'general.slang',
                                output : '@BASENAME@.rchitshader',
                                command : ['slangc', '-fvk-use-gl-layout', '-O0', '-target' , 'spirv', '-profile', 'glsl_460', '@INPUT@', '-entry', 'chit_main', '-stage', 'closesthit', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_ray_tracing_miss',
                                input : 'general.slang',
                                output : '@BASENAME@.rmissshader',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'glsl_460', '@INPUT@', '-entry', 'miss_main', '-stage', 'miss', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_ray_tracing_shadow_chit',
                                input : ['shadow.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rchitshader',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT0@', '-entry', 'chit_main', '-stage', 'closesthit', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_ray_tracing_shadow_miss',
                                input : ['shadow.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rmissshader',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT0@', '-entry', 'miss_main', '-stage', 'miss', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_ray_tracing_ao_chit',
                                input : ['ambient_occlusion.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rchitshader',
                                command : ['slangc', '-O0', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT0@', '-entry', 'chit_main', '-stage', 'closesthit', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_ray_tracing_ao_miss',
                                input : ['ambient_occlusion.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rmissshader',
                                command : ['slangc', '-target' , 'spirv', '-profile', 'spirv_1_6', '@INPUT0@', '-entry', 'miss_main', '-stage', 'miss', '-o', '@OUTPUT@'],
)