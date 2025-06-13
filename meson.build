project(
  'tint',
  ['c', 'cpp'],
  version: '0.0.0',
  default_options: ['c_std=c11', 'cpp_std=c++23'],
)

qt6 = import('qt6')
mocs = qt6.compile_moc(headers: [
  'include/plugin-clock.h',
])

wlr_protocols = dependency('wlr-protocols')
wlr_protocol_dir = wlr_protocols.get_variable('pkgdatadir')

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
  wayland_scanner_c.process(wlr_protocol_dir / 'unstable/wlr-foreign-toplevel-management-unstable-v1.xml'),
  wayland_scanner_h.process(wlr_protocol_dir / 'unstable/wlr-foreign-toplevel-management-unstable-v1.xml'),
]

srcs = [
  mocs,
  protos,
  'conf.cpp',
  'main.cpp',
  'panel.cpp',
  'plugin-clock.cpp',
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
  dependency('libsfdo-basedir'),
  dependency('libsfdo-desktop'),
  dependency('libsfdo-icon'),
]

incs = include_directories('include')

executable(
  meson.project_name(),
  srcs,
  include_directories: [incs],
  dependencies: deps,
)
