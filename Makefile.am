AUTOMAKE_OPTIONS = foreign

SUBDIRS = src data po

DISTCLEANFILES =	intltool-extract \
			intltool-merge \
			intltool-update \
			po/.intltool-merge-cache

INTLTOOL_FILES =	intltool-extract.in \
			intltool-merge.in \
			intltool-update.in

EXTRA_DIST = README.md AUTHORS

UPDATE_DESKTOP = update-desktop-database -q || :
UPDATE_ICON = gtk-update-icon-cache -q $(datadir)/icons/hicolor/ || :

install-data-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)

uninstall-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)
