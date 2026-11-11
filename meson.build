project(
  'gmarkdown',
  'c',
  version: '0.1.0',
  meson_version: '>=1.2.0',
  license: 'BSD-2-Clause',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'werror=true',
  ],
)

gnome = import('gnome')
i18n = import('i18n')

gtk = dependency('gtk4', version: '>=4.14')
adwaita = dependency('libadwaita-1', version: '>=1.5')
webkit = dependency('webkitgtk-6.0')
glib = dependency('gio-2.0')

cmark = dependency('libcmark-gfm', required: false)
cmark_ext = dependency('libcmark-gfmextensions', required: false)
if not cmark_ext.found()
  cmark_ext = dependency('libcmark-gfm-extensions', required: false)
endif
if not cmark.found()
  cmark_sub = subproject('cmark-gfm')
  cmark = cmark_sub.get_variable('cmark_gfm_dep')
  cmark_ext = cmark_sub.get_variable('cmark_gfm_ext_dep')
endif

conf = configuration_data()
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('GMARKDOWN_PKGDATADIR',
  get_option('prefix') / get_option('datadir') / meson.project_name())
conf.set_quoted('GMARKDOWN_DEV_DATADIR', meson.current_source_dir() / 'data')
configure_file(output: 'config.h', configuration: conf)

resources = gnome.compile_resources(
  'gmarkdown-resources',
  'data/gmarkdown.gresource.xml',
  source_dir: 'data',
  c_name: 'gmarkdown_resources',
)

cc = meson.get_compiler('c')
math = cc.find_library('m', required: false)

gmarkdown_sources = files(
  'src/main.c',
  'src/window.c',
  'src/markdown.c',
  'src/print.c',
)

executable(
  'gmarkdown',
  gmarkdown_sources,
  resources,
  dependencies: [gtk, adwaita, webkit, glib, cmark, cmark_ext, math],
  install: true,
  include_directories: include_directories('src'),
)

desktop_file = configure_file(
  input: 'data/business.generic.GMarkdown.desktop.in',
  output: 'business.generic.GMarkdown.desktop',
  copy: true,
)
install_data(
  desktop_file,
  install_dir: get_option('datadir') / 'applications',
)

install_data(
  'data/icons/hicolor/scalable/apps/business.generic.GMarkdown.svg',
  install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps',
)

install_data(
  'data/vendor/mermaid.min.js',
  install_dir: get_option('datadir') / meson.project_name() / 'vendor',
)

test_markdown = executable(
  'test-markdown',
  files('tests/test-markdown.c', 'src/markdown.c'),
  dependencies: [glib, cmark, cmark_ext],
  include_directories: include_directories('src'),
)
test(
  'markdown-gfm',
  test_markdown,
  env: {
    'GMARKDOWN_FOREGROUND': '1',
    'GMARKDOWN_FIXTURE': meson.current_source_dir() / 'tests/fixtures/gfm.md',
  },
)

