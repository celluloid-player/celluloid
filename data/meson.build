
schemadir = join_paths(get_option('datadir'), 'glib-2.0/schemas')
install_data([
  'io.github.celluloid_player.Celluloid.gschema.xml',
], install_dir: schemadir)

metainfo = i18n.merge_file(
  input: 'io.github.celluloid_player.Celluloid.appdata.xml.in',
  output: 'io.github.celluloid_player.Celluloid.appdata.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

desktop = i18n.merge_file(
  input: 'io.github.celluloid_player.Celluloid.desktop.in',
  output: 'io.github.celluloid_player.Celluloid.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

icondir = join_paths(get_option('datadir'), 'icons/hicolor')
install_data('io.github.celluloid_player.Celluloid.svg',
  install_dir: join_paths(icondir, 'scalable/apps')
)

install_data('io.github.celluloid_player.Celluloid-symbolic.svg',
  install_dir: join_paths(icondir, 'symbolic/apps')
)

service_conf = configuration_data()
service_conf.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(
  input: 'io.github.celluloid_player.Celluloid.service.in',
  output: 'io.github.celluloid_player.Celluloid.service',
  configuration: service_conf,
  install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services')
)

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test('Validate appstream file', appstreamcli, args: ['validate', '--no-net', metainfo])
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

# Compile schemas for devenv etc.
gnome.compile_schemas()
