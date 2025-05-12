project(
  'tint',
  ['c', 'cpp'],
  version: '0.0.0',
  default_options: ['c_std=c11', 'cpp_std=c++23'],
)

qt6 = import('qt6')

wayland_scanner = find_program('wayland-scanner')
wayland_scanner_c = generator(
  wayland_scanner,
  output: '@BASENAME@.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)
wayland_scanner_h = generator(
  wayland_scanner,
  output: '@BASENAME@.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)
protos = [
  wayland_scanner_c.process('wlr-foreign-toplevel-management-unstable-v1.xml'),
  wayland_scanner_h.process('wlr-foreign-toplevel-management-unstable-v1.xml'),
]

srcs = [
  protos,
  'conf.cpp',
  'main.cpp',
  'panel.cpp',
  'taskbar.cpp',
]

deps = [
  dependency(
    'qt6',
    modules: [
      'Core',
      'Gui',
      'Widgets',
      'WaylandClient'
    ],
    private_headers: true),
  dependency('LayerShellQt', modules: ['LayerShellQt::Interface']),
  dependency('wayland-client'),
]

executable(
  meson.project_name(),
  srcs,
  dependencies: deps,
)
