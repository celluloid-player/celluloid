
schemadir = join_paths(get_option('datadir'), 'glib-2.0/schemas')
install_data([
  'io.github.GnomeMpv.gschema.xml',
  'org.gnome-mpv.gschema.xml'
], install_dir: schemadir)

appdata = i18n.merge_file(
  input: 'io.github.GnomeMpv.appdata.xml.in',
  output: 'io.github.GnomeMpv.appdata.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata')
)

desktop = i18n.merge_file(
  input: 'io.github.GnomeMpv.desktop.in',
  output: 'io.github.GnomeMpv.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

icondir = join_paths(get_option('datadir'), 'icons/hicolor')
install_data('io.github.GnomeMpv.svg',
  install_dir: join_paths(icondir, 'scalable/apps')
)

install_data('io.github.GnomeMpv-symbolic.svg',
  install_dir: join_paths(icondir, 'symbolic/apps')
)

service_conf = configuration_data()
service_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(
  input: 'io.github.GnomeMpv.service.in',
  output: 'io.github.GnomeMpv.service',
  configuration: service_conf,
  install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services')
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util, args: ['validate', appdata])
endif

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test('Validate desktop file', desktop_file_validate, args: [desktop])
endif

glib_compile_schemas = find_program('glib-compile-schemas', required: false)
if glib_compile_schemas.found()
  test('Validate schema file', glib_compile_schemas,
    args: ['--strict', '--dry-run', meson.current_source_dir()]
  )
endif
