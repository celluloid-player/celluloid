gsettings_SCHEMAS = org.gnome-mpv.gschema.xml

@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_DATA = gnome-mpv.desktop

icondir = $(datadir)/icons/hicolor/scalable/apps
icon_DATA = gnome-mpv.svg

symicondir = $(datadir)/icons/hicolor/symbolic/apps
symicon_DATA = gnome-mpv-symbolic.svg

EXTRA_DIST =	$(gsettings_SCHEMAS) mpris_gdbus.xml gnome-mpv.svg \
		gnome-mpv.desktop
