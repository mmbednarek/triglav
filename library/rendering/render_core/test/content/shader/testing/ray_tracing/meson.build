render_core_test_file_deps += custom_target(input   : 'basic.slang',
                                            output  : '@BASENAME@.rgenshader',
                                            command : slang_compile_rgen,
)

render_core_test_file_deps += custom_target(input   : 'basic.slang',
                                            output  : '@BASENAME@.rmissshader',
                                            command : slang_compile_rmiss,
)

render_core_test_file_deps += custom_target(input   : 'basic.slang',
                                            output  : '@BASENAME@.rchitshader',
                                            command : slang_compile_rchit,
)
