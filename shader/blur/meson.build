shader_targets += custom_target('shader_blur_compute',
    input: 'blur.slang',
    output  : 'compute.spv',
    command : ['slangc', '-target', 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-o', '@OUTPUT@'],
)

shader_targets += custom_target('shader_blur_compute_sc',
    input: 'blur.slang',
    output  : 'compute_sc.spv',
    command : ['slangc', '-target', 'spirv', '-profile', 'spirv_1_6', '@INPUT@', '-entry', 'cs_main', '-stage', 'compute', '-O3', '-DSINGLE_CHANNEL', '-o', '@OUTPUT@'],
)
