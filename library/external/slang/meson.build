tp_slang_dir = ''

if host_machine.system() == 'linux' and host_machine.cpu_family() == 'x86_64'
    tp_slang_dir = 'linux_x86_64'
elif  host_machine.system() == 'windows' and host_machine.cpu_family() == 'x86_64'
    tp_slang_dir = 'windows_x86_64'
else
    error('unsupported platform for slang')
endif

tp_slang_lib_dir = meson.current_source_dir() + '/' + tp_slang_dir + '/lib'

tp_slang = declare_dependency(
    dependencies : [
        cpp.find_library('slang', dirs : tp_slang_lib_dir),
        cpp.find_library('slang-compiler', dirs : tp_slang_lib_dir),
    ],
    include_directories : include_directories([tp_slang_dir + '/include']),
)
