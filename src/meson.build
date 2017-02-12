gnome = import('gnome')

sources = [
  'gmpv_main.c',
  'gmpv_actionctl.c',
  'gmpv_application.c',
  'gmpv_common.c',
  'gmpv_control_box.c',
  'gmpv_file_chooser.c',
  'gmpv_header_bar.c',
  'gmpv_inputctl.c',
  'gmpv_main_window.c',
  'gmpv_menu.c',
  'gmpv_mpv.c',
  'gmpv_mpv_opt.c',
  'gmpv_open_loc_dialog.c',
  'gmpv_playbackctl.c',
  'gmpv_playlist.c',
  'gmpv_playlist_widget.c',
  'gmpv_plugins_manager.c',
  'gmpv_plugins_manager_item.c',
  'gmpv_pref_dialog.c',
  'gmpv_seek_bar.c',
  'gmpv_track.c',
  'gmpv_video_area.c',
  'gmpv_mpv_wrapper.c',
  'media_keys/gmpv_media_keys.c',

  'mpris/gmpv_mpris.c',
  'mpris/gmpv_mpris_base.c',
  'mpris/gmpv_mpris_player.c'
]

sources += custom_target('authors',
  input: '../AUTHORS',
  output: 'gmpv_authors.h',
  command: [
    find_program('generate_authors.py'),
    '@INPUT@', '@OUTPUT@'
  ]
)

sources += gnome.genmarshal('gmpv_marshal',
  sources: '../data/gmpv_marshal.lst',
  prefix: 'g_cclosure_gen_marshal',
)

libgtk = dependency('gtk+-3.0', version: '>= 3.18')
localedir = join_paths(get_option('prefix'), get_option('localedir'))
cflags = [
  '-DG_SETTINGS_ENABLE_BACKEND',
  '-DPACKAGE_LOCALEDIR="@0@"'.format(localedir),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_44',
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_44',
]

if get_option('buildtype') == 'release'
  cflags += [
    '-DG_DISABLE_ASSERT=1',
    '-DG_DISABLE_CHECKS=1',
    '-DG_DISABLE_CAST_CHECKS=1'
  ]
endif

if libgtk.version().version_compare('>= 3.20')
  cflags += [
    '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_20',
    '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_20'
  ]
  sources += 'gmpv_shortcuts_window.c'
else
  cflags += [
    '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_18',
    '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_3_18'
  ]
endif


if target_machine.system() == 'windows'
  libgio = dependency('gio-2.0', version: '>= 2.44')
else
  libgio = dependency('gio-unix-2.0', version: '>= 2.44')
endif

generated_gdbus_sources = gnome.gdbus_codegen(
  'gmpv_mpris_gdbus',
  '../data/gmpv_mpris_gdbus.xml',
  namespace: 'gmpv_mpris'
)

# FIXME: gdbus outputs code with warnings, so compile that
# with extra args
lib_mpris_gdbus = static_library('mpris-gdbus',
  generated_gdbus_sources,
  c_args: cflags + ['-Wno-conversion'],
  dependencies: libgio,
  include_directories: include_directories('..'), # FIXME
)


executable('gnome-mpv', sources,
  dependencies: [
    libgtk,
    libgio,
    dependency('mpv', version: '>= 1.20'),
    dependency('epoxy')
  ],
  link_with: lib_mpris_gdbus,
  c_args: cflags,
  install: true
)
