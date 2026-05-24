project('PTSimulation', 'cpp',
    version : '0.1.0',
    default_options : ['cpp_std=c++20', 'buildtype=debug', 'b_ndebug=if-release'])

vulkan = dependency('vulkan')
glm = dependency('glm')
eigen = dependency('eigen3')
glfw = dependency('glfw3')

# When at some point I want to use rocm, I add this to the relevant meson build file, and compile the relevant .hip files with hipcc
# hipcc = find_program('/opt/rocm-7.2.3/bin/hipcc', required: true)
# rocm_include = include_directories('/opt/rocm-7.2.3/include')

# Instead of all dependencies here, at some point I probably should split the code into the 'common_types', 'compute' and 'renderer' folders. commmon_types defines the data types / structs that both the renderer and compute interact with. 
# subdir('common_types')
# subdir('compute_core')
# subdir('renderer') 

executable('PTSimulation', 'src/main.cpp', 
    dependencies: [vulkan, glm, glfw, eigen],
    install: false)