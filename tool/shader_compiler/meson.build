shader_compiler_sources = files([
                               'src/CommandLine.cpp',
                               'src/CommandLine.hpp',
                               'src/Compiler.cpp',
                               'src/Compiler.hpp',
                               'src/Main.cpp',
                           ])

shader_compiler_deps = [
    core,
    asset,
    render_core,
    tp_slang,
]

shader_compiler = executable('shader_compiler',
                        sources : shader_compiler_sources,
                        dependencies : shader_compiler_deps,
                        install : true,
                        install_dir : 'bin',
)
