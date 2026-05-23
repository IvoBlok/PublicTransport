project('PTSimulation', 'cpp',
    version : '0.1.0',
    default_options : ['cpp_std=c++20'])

vulkan = dependency('vulkan')

executable('PTSimulation', 'src/main.cpp', 
    dependencies: [vulkan],
    install: false)