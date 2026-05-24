
glm = dependency('glm')
eigen = dependency('eigen3')

common_types_inc = include_directories('include')

common_types_dep = declare_dependency(
    include_directories : common_types_inc,
    dependencies : [glm, eigen]
)



# once this has cpp files: 
# common_types_lib = static_library('common_types',
#     sources : ['src/spline.cpp'],
#     dependencies : [glm_dep]
# )
# 
# common_types_dep = declare_dependency(
#     include_directories : common_types_inc,
#     link_with : common_types_lib,
#     dependencies : [glm_dep]
# )