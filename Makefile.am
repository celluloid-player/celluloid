AUTOMAKE_OPTIONS = foreign

SUBDIRS=src po

DISTCLEANFILES =	intltool-extract \
			intltool-merge \
			intltool-update \
			po/.intltool-merge-cache

INTLTOOL_FILES =	intltool-extract.in \
			intltool-merge.in \
			intltool-update.in

UPDATE_DESKTOP = update-desktop-database -q || :
UPDATE_ICON = gtk-update-icon-cache -q $(datadir)/icons/hicolor/ || :

desktopdir = $(datadir)/applications
desktop_DATA = gnome-mpv.desktop

icondir = $(datadir)/icons/hicolor/scalable/apps
icon_DATA = gnome-mpv.svg

install-data-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)

uninstall-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)
