stbi_args = []

if meson.get_compiler('cpp').get_id() == 'msvc'
    stbi_args += '/wd4244'
endif

stbi_lib = static_library('stbi',
    sources: files(['src/stbi_impl.c']),
    include_directories: include_directories(['include/stbi']),
    c_args: stbi_args,
)

stbi = declare_dependency(link_with: [stbi_lib], include_directories: include_directories(['include']))
