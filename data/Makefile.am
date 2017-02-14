gsettings_SCHEMAS = io.github.GnomeMpv.gschema.xml org.gnome-mpv.gschema.xml

@GSETTINGS_RULES@

appstream_in_files = io.github.GnomeMpv.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)

%.appdata.xml: %.appdata.xml.in
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

@APPSTREAM_XML_RULES@

desktopdir = $(datadir)/applications
desktop_in_files = io.github.GnomeMpv.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

icondir = $(datadir)/icons/hicolor/scalable/apps
icon_DATA = gnome-mpv.svg

symicondir = $(datadir)/icons/hicolor/symbolic/apps
symicon_DATA = gnome-mpv-symbolic.svg

EXTRA_DIST =	$(appstream_in_files) $(desktop_in_files) \
		gnome-mpv.svg gnome-mpv-symbolic.svg \
		org.gnome-mpv.gschema.xml \
		io.github.GnomeMpv.gschema.xml \
		gmpv_mpris_gdbus.xml gmpv_marshal.lst \
		meson.build

DISTCLEANFILES = $(appstream_XML) $(desktop_DATA)
