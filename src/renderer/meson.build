
# handle shaders
glslc = find_program('glslc', required : true)

shader_sources = [
    'shaders/line.vert',
    'shaders/line.frag'
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

# handle other dependencies
vulkan = dependency('vulkan')
glfw = dependency('glfw3')

tracy = dependency('tracy', required: true, static: true)

renderer_inc = include_directories('include')

renderer_src = [
    'src/device.cpp',
    'src/swapChain.cpp',
    'src/frameManager.cpp',
    'src/renderPass.cpp',
    'src/pipeline.cpp',
    'src/renderEngine.cpp',
    'src/renderables/line.cpp'
]

renderer_lib = static_library('renderer',
    renderer_src,
    include_directories : renderer_inc,
    dependencies : [
        translation_dep,
        vulkan,
        glfw,
        tracy
    ]
)

renderer_dep = declare_dependency(
    include_directories : renderer_inc,
    link_with : renderer_lib,
    dependencies : [translation_dep, tracy]
)