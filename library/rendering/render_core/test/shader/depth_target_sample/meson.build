render_core_test_file_deps += custom_target(input   : 'draw.slang',
                                            output  : '@BASENAME@.vshader',
                                            command : slang_compile_vs,
)

render_core_test_file_deps += custom_target(input   : 'draw.slang',
                                            output  : '@BASENAME@.fshader',
                                            command : slang_compile_fs,
)

render_core_test_file_deps += custom_target(input   : 'sample.slang',
                                            output  : '@BASENAME@.fshader',
                                            command : slang_compile_fs,
)
