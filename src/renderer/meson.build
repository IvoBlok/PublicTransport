
vulkan = dependency('vulkan')
glfw = dependency('glfw3')

renderer_inc = include_directories('include')

renderer_src = [
    'src/renderer.cpp'
]

renderer_lib = static_library('renderer',
    renderer_src,
    include_directories : renderer_inc,
    dependencies : [
        common_types_dep,
        vulkan,
        glfw
    ]
)

renderer_dep = declare_dependency(
    include_directories : renderer_inc,
    link_with : renderer_lib,
    dependencies : [common_types_dep]
)