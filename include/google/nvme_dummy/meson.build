project(
  'nvme_dummy',
  'cpp',
  version: '0.1', meson_version: '>=0.61.0',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++20'
  ],
)

cpp = meson.get_compiler('cpp')
sdbusplus_dep = dependency('sdbusplus')

fmt_dep = dependency('fmt', required: false)
if not fmt_dep.found()
  fmt_proj = import('cmake').subproject(
    'fmt',
    cmake_options: [
      '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
      '-DMASTER_PROJECT=OFF'
    ],
    required: false)
  assert(fmt_proj.found(), 'fmtlib is required')
  fmt_dep = fmt_proj.dependency('fmt')
endif

includes = include_directories('.')

nvme_pre = declare_dependency(
  include_directories: includes,
  dependencies: [
    fmt_dep,
    sdbusplus_dep,
    dependency('threads')
  ]
)

nvme_lib = static_library(
  'nvme',
  include_directories : includes,
  implicit_include_directories: false,
  dependencies: nvme_pre,
)

nvme_dep = declare_dependency(
  dependencies: nvme_pre,
  link_with: nvme_lib
)

executable(
  'nvme',
  'nvme.cpp',
  implicit_include_directories: false,
  dependencies: nvme_dep,
  install: true,
  install_dir: get_option('libexecdir'),
)
