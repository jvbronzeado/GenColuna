project('GenColunas', ['c', 'cpp'])

cc = meson.get_compiler('cpp')

gurobi_inc_path = meson.get_external_property('gurobi_inc', native: true)
gurobi_lib_path = meson.get_external_property('gurobi_lib', native: true)

gurobi_include = include_directories(gurobi_inc_path)
gurobi_lib = cc.find_library('gurobi_c++', dirs: gurobi_lib_path)

gurobi_cpp_lib = cc.find_library('gurobi_c++', dirs: gurobi_lib_path)
gurobi_c_lib = cc.find_library('gurobi130', dirs: gurobi_lib_path)

gurobi_dep = declare_dependency(
  dependencies: [gurobi_cpp_lib, gurobi_c_lib],
  include_directories: gurobi_include
)

includes = []

sources = []
subdir('src')

executable('GenColunas',
  sources,
  build_subdir: 'bin',
  dependencies: [gurobi_dep]
)
