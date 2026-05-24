project('PTSimulation', 'cpp',
    version : '0.1.0',
    default_options : ['cpp_std=c++20', 'buildtype=debug', 'b_ndebug=if-release'])

# When at some point I want to use rocm, I add this to the relevant meson build file, and compile the relevant .hip files with hipcc
# hipcc = find_program('/opt/rocm-7.2.3/bin/hipcc', required: true)
# rocm_include = include_directories('/opt/rocm-7.2.3/include')

subdir('src/common_types')
subdir('src/compute')
subdir('src/renderer') 

executable('PTSimulation', 'src/main.cpp', 
    dependencies: [common_types_dep, compute_dep, renderer_dep],
    install: false)