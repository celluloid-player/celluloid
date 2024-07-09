gnome = import('gnome')

sources = [
  'celluloid-application.c',
  'celluloid-common.c',
  'celluloid-control-box.c',
  'celluloid-controller.c',
  'celluloid-controller-actions.c',
  'celluloid-controller-input.c',
  'celluloid-file-chooser.c',
  'celluloid-file-chooser-button.c',
  'celluloid-header-bar.c',
  'celluloid-main.c',
  'celluloid-main-window.c',
  'celluloid-menu.c',
  'celluloid-metadata-cache.c',
  'celluloid-model.c',
  'celluloid-mpv.c',
  'celluloid-open-location-dialog.c',
  'celluloid-option-parser.c',
  'celluloid-player.c',
  'celluloid-player-options.c',
  'celluloid-playlist-widget.c',
  'celluloid-playlist-item.c',
  'celluloid-playlist-model.c',
  'celluloid-plugins-manager.c',
  'celluloid-plugins-manager-item.c',
  'celluloid-preferences-dialog.c',
  'celluloid-seek-bar.c',
  'celluloid-shortcuts-window.c',
  'celluloid-time-label.c',
  'celluloid-video-area.c',
  'celluloid-view.c',

  'mpris/celluloid-mpris.c',
  'mpris/celluloid-mpris-module.c',
  'mpris/celluloid-mpris-base.c',
  'mpris/celluloid-mpris-player.c',
  'mpris/celluloid-mpris-track-list.c'
]

sources += custom_target('authors',
  input: '../AUTHORS',
  output: 'celluloid-authors.h',
  command: [
    find_program('generate-authors.py'),
    '@INPUT@', '@OUTPUT@'
  ]
)

libgtk = dependency('gtk4', version: '>=4.6.1')
localedir = join_paths(get_option('prefix'), get_option('localedir'))
cflags = [
  '-DG_SETTINGS_ENABLE_BACKEND',
  '-DPACKAGE_LOCALEDIR="@0@"'.format(localedir),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_66',
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_66',
  '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_6',
  '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_6'
]

if get_option('buildtype') == 'release'
  cflags += [
    '-DG_DISABLE_ASSERT=1',
    '-DG_DISABLE_CHECKS=1',
    '-DG_DISABLE_CAST_CHECKS=1'
  ]
endif

if target_machine.system() == 'windows'
  libgio = dependency('gio-2.0', version: '>= 2.44')
else
  libgio = dependency('gio-unix-2.0', version: '>= 2.44')
endif

generated_gdbus_sources = gnome.gdbus_codegen(
  'celluloid-mpris-gdbus',
  '../data/celluloid-mpris-gdbus.xml',
  namespace: 'celluloid_mpris'
)

generated_marshal_sources = gnome.genmarshal('celluloid-marshal',
  sources: '../data/celluloid-marshal.lst',
  prefix: 'g_cclosure_gen_marshal',
)

# NOTE: All of these are fixed in GLib 2.54 and Meson 0.42.0 and can be
# removed once version requirements are bumped
includes = []
extra_libs = []
if libgio.version().version_compare('< 2.52.0')
  # We want to be warning free on old versions:
  # https://bugzilla.gnome.org/show_bug.cgi?id=778581
  extra_libs += static_library('mpris-gdbus',
    generated_gdbus_sources,
    c_args: cflags + ['-Wno-conversion'],
    dependencies: libgio,
    include_directories: include_directories('..'),
  )
  # Created includes of `src/...`:
  # https://bugzilla.gnome.org/show_bug.cgi?id=778801
  includes = include_directories('..')
elif meson.version().version_compare('== 0.46.0')
  # https://github.com/mesonbuild/meson/issues/3488
  includes = include_directories('..')
  sources += generated_gdbus_sources
else
  sources += generated_gdbus_sources
endif

if libgio.version().version_compare('< 2.54.0')
  # Silence warning about missing prototypes in genmarshal-generated files
  extra_libs += static_library('marshal',
    generated_marshal_sources,
    c_args: cflags + ['-Wno-missing-prototypes'],
    dependencies: libgio,
    include_directories: include_directories('..')
  )
else
  sources += generated_marshal_sources
endif

executable('celluloid', sources,
  dependencies: [
    libgtk,
    libgio,
    meson.get_compiler('c').find_library('m', required: false),
    dependency('mpv', version: '>= 1.107'),
    dependency('libadwaita-1', version: '>= 1.4.0'),
    dependency('epoxy')
  ],
  link_with: extra_libs,
  include_directories: includes,
  c_args: cflags,
  install: true
)
