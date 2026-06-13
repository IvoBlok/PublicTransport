project('PTSimulation', 'cpp',
    version : '0.1.0',
    default_options : ['cpp_std=c++20', 'buildtype=debug', 'b_ndebug=if-release'])

# When at some point I want to use rocm, I add this to the relevant meson build file, and compile the relevant .hip files with hipcc
# hipcc = find_program('/opt/rocm-7.2.3/bin/hipcc', required: true)
# rocm_include = include_directories('/opt/rocm-7.2.3/include')

if get_option('tracy_enable') and get_option('buildtype') != 'debugoptimized'
    warning('Profiling builds should set --buildtype=debugoptimized')
endif

subdir('src/compute')
subdir('src/translation')
subdir('src/renderer') 

executable('PTSimulation', 'src/main.cpp', 
    dependencies: [compute_dep, translation_dep, renderer_dep],
    install: false)