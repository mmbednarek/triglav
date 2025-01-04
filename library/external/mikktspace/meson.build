mikktspace_args = []

if meson.get_compiler('cpp').get_id() == 'clang'
    mikktspace_args += '-Wno-unused-but-set-variable'
endif

mikktspace_lib = static_library('mikktspace',
    sources: files(['src/mikktspace.c']),
    include_directories: include_directories(['include/mikktspace']),
    c_args: mikktspace_args,
)

mikktspace = declare_dependency(link_with: [mikktspace_lib], include_directories: include_directories(['include']))
