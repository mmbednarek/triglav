ray_generation_commands = [
    shader_compiler,
    '@INPUT0@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--entry', 'rgen_main',
    '--stage', 'ray_generation',
    '-o', '@OUTPUT@'
]

closest_hit_commands = [
    shader_compiler,
    '@INPUT0@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--entry', 'chit_main',
    '--stage', 'closest_hit',
    '-o', '@OUTPUT@'
]

miss_commands = [
    shader_compiler,
    '@INPUT0@',
    shader_build_flags,
    '-I', shader_library_dir,
    '--entry', 'miss_main',
    '--stage', 'miss',
    '-o', '@OUTPUT@'
]

shader_targets += custom_target('shader_ray_tracing_gen',
                                input : ['ray_generation.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rgenshader',
                                command : ray_generation_commands,
)

shader_targets += custom_target('shader_ray_tracing_chit',
                                input : 'general.slang',
                                output : '@BASENAME@.rchitshader',
                                command : closest_hit_commands,
)

shader_targets += custom_target('shader_ray_tracing_miss',
                                input : 'general.slang',
                                output : '@BASENAME@.rmissshader',
                                command : miss_commands,
)

shader_targets += custom_target('shader_ray_tracing_shadow_chit',
                                input : ['shadow.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rchitshader',
                                command : closest_hit_commands,
)

shader_targets += custom_target('shader_ray_tracing_shadow_miss',
                                input : ['shadow.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rmissshader',
                                command : miss_commands,
)

shader_targets += custom_target('shader_ray_tracing_ao_chit',
                                input : ['ambient_occlusion.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rchitshader',
                                command : closest_hit_commands,
)

shader_targets += custom_target('shader_ray_tracing_ao_miss',
                                input : ['ambient_occlusion.slang', 'ray_payload.slang', 'ray_tracing.slang'],
                                output : '@BASENAME0@.rmissshader',
                                command : miss_commands,
)