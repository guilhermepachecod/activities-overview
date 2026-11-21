ao_sources = [
  'main.c',
  'overview.c',
  'background.c',
]

ao_deps = [
  dependency('glib-2.0', version: '>= 2.66'),
  dependency('gio-2.0'),
  dependency('gtk+-3.0', version: '>= 3.22'),
]

executable(
  'activities-overview',
  ao_sources,
  dependencies: ao_deps,
  c_args: ['-DPACKAGE_VERSION="' + meson.project_version() + '"'],
  install: true,
)
