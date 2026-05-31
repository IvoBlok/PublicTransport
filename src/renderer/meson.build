
# handle shaders
glslc = find_program('glslc', required : true)

shader_sources = [
    'shaders/testShader.vert',
    'shaders/testShader.frag'
]

shader_targets = []
foreach src : shader_sources
    src_name = src.split('/')[-1]

    shader_targets += custom_target(
        'compiled_' + src_name,
        input : src,
        output : src_name + '.spv',
        command : [glslc, '@INPUT@', '-o', '@OUTPUT@'],
        build_by_default : true,
        install : false
    )
endforeach

# handle dependencies
vulkan = dependency('vulkan')
glfw = dependency('glfw3')


renderer_inc = include_directories('include')

renderer_src = [
    'src/coreTypes.cpp',
    'src/renderer.cpp',
    'src/objectTypes/line.cpp'
]

renderer_lib = static_library('renderer',
    renderer_src,
    include_directories : renderer_inc,
    dependencies : [
        translation_dep,
        vulkan,
        glfw
    ]
)

renderer_dep = declare_dependency(
    include_directories : renderer_inc,
    link_with : renderer_lib,
    dependencies : [translation_dep]
)