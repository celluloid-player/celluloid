gsettings_SCHEMAS = io.github.GnomeMpv.gschema.xml org.gnome-mpv.gschema.xml

@GSETTINGS_RULES@

appstream_in_files = io.github.GnomeMpv.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@
@APPSTREAM_XML_RULES@

desktopdir = $(datadir)/applications
desktop_in_files = io.github.GnomeMpv.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

icondir = $(datadir)/icons/hicolor/scalable/apps
icon_DATA = gnome-mpv.svg

symicondir = $(datadir)/icons/hicolor/symbolic/apps
symicon_DATA = gnome-mpv-symbolic.svg

EXTRA_DIST =	$(appstream_in_files) $(desktop_in_files) \
		gnome-mpv.svg gnome-mpv-symbolic.svg \
		io.github.GnomeMpv.gschema.xml.in org.gnome-mpv.gschema.xml.in \
		mpris_gdbus.xml

DISTCLEANFILES = $(appstream_XML) $(gsettings_SCHEMAS) $(desktop_DATA)
