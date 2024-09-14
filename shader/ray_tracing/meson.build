shader_targets += custom_target('shader_ray_tracing_gen',
                                input : 'ray_generation.glsl',
                                output : '@BASENAME@.spv',
                                command : compile_raygen_cmds,
)

shader_targets += custom_target('shader_ray_tracing_chit',
                                input : 'closest_hit.glsl',
                                output : '@BASENAME@.spv',
                                command : compile_raychit_cmds,
)

shader_targets += custom_target('shader_ray_tracing_miss',
                                input : 'miss.glsl',
                                output : '@BASENAME@.spv',
                                command : compile_raymiss_cmds,
)
