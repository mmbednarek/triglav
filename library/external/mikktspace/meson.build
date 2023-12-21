mikktspace_lib = static_library('mikktspace',
    sources: files(['src/mikktspace.c']),
    include_directories: include_directories(['include/mikktspace']),
)

mikktspace = declare_dependency(link_with: [mikktspace_lib], include_directories: include_directories(['include']))
