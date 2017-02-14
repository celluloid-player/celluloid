AUTOMAKE_OPTIONS = foreign

SUBDIRS = src data po

EXTRA_DIST = README.md AUTHORS meson.build meson_post_install.py po/meson.build

UPDATE_DESKTOP = update-desktop-database -q || :
UPDATE_ICON = gtk-update-icon-cache -q $(datadir)/icons/hicolor/ || :

# Ensure that the dist tarball does not contain own/group from build system
TAR_OPTIONS = --owner=0 --group=0
export TAR_OPTIONS

install-data-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)

uninstall-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)
