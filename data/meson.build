
schemadir = join_paths(get_option('datadir'), 'glib-2.0/schemas')
install_data([
  'io.github.GnomeMpv.gschema.xml',
  'org.gnome-mpv.gschema.xml'
], install_dir: schemadir)

i18n.merge_file(
  input: 'io.github.GnomeMpv.appdata.xml.in',
  output: 'io.github.GnomeMpv.appdata.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata')
)

i18n.merge_file(
  input: 'io.github.GnomeMpv.desktop.in',
  output: 'io.github.GnomeMpv.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications')
)

icondir = join_paths(get_option('datadir'), 'icons/hicolor')
install_data('gnome-mpv.svg',
  install_dir: join_paths(icondir, 'scalable/apps')
)

install_data('gnome-mpv-symbolic.svg',
  install_dir: join_paths(icondir, 'symbolic/apps')
)

service_conf = configuration_data()
service_conf.set('bindir', get_option('bindir'))

configure_file(
  input: 'io.github.GnomeMpv.service.in',
  output: 'io.github.GnomeMpv.service',
  configuration: service_conf,
  install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services')
)
