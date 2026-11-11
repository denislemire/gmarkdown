# Bundled GitHub cmark-gfm 0.29.0.gfm.13 for hosts without libcmark-gfm.
# SPDX-License-Identifier: BSD-2-Clause

cc = meson.get_compiler('c')

cmark_inc = include_directories('src', 'extensions')

cmark_c_args = [
  '-DCMARK_GFM_STATIC_DEFINE',
  '-DCMARK_GFM_EXTENSIONS_STATIC_DEFINE',
]

cmark_lib = static_library(
  'cmark-gfm',
  files(
    'src/cmark.c',
    'src/node.c',
    'src/iterator.c',
    'src/blocks.c',
    'src/inlines.c',
    'src/scanners.c',
    'src/utf8.c',
    'src/buffer.c',
    'src/references.c',
    'src/footnotes.c',
    'src/map.c',
    'src/render.c',
    'src/man.c',
    'src/xml.c',
    'src/html.c',
    'src/commonmark.c',
    'src/plaintext.c',
    'src/latex.c',
    'src/houdini_href_e.c',
    'src/houdini_html_e.c',
    'src/houdini_html_u.c',
    'src/cmark_ctype.c',
    'src/arena.c',
    'src/linked_list.c',
    'src/syntax_extension.c',
    'src/registry.c',
    'src/plugin.c',
  ),
  include_directories: cmark_inc,
  c_args: cmark_c_args,
  override_options: ['warning_level=1', 'werror=false'],
)

cmark_ext_lib = static_library(
  'cmark-gfm-extensions',
  files(
    'extensions/core-extensions.c',
    'extensions/table.c',
    'extensions/strikethrough.c',
    'extensions/autolink.c',
    'extensions/tagfilter.c',
    'extensions/ext_scanners.c',
    'extensions/tasklist.c',
  ),
  include_directories: cmark_inc,
  c_args: cmark_c_args,
  link_with: cmark_lib,
  override_options: ['warning_level=1', 'werror=false'],
)

cmark_gfm_dep = declare_dependency(
  link_with: [cmark_lib, cmark_ext_lib],
  include_directories: cmark_inc,
  compile_args: cmark_c_args,
)

cmark_gfm_ext_dep = cmark_gfm_dep
