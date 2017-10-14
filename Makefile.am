AUTOMAKE_OPTIONS = foreign

SUBDIRS = src data docs po

EXTRA_DIST = README.md AUTHORS meson.build docs/meson.build meson_post_install.py po/meson.build

UPDATE_DESKTOP = update-desktop-database -q || :
UPDATE_ICON = gtk-update-icon-cache -q $(datadir)/icons/hicolor/ || :

# Ensure that the dist tarball does not contain own/group from build system
TAR_OPTIONS = --owner=0 --group=0
export TAR_OPTIONS

# Extract version numbers in appdata XML and the main meson build file
APPDATA_VERSION = $(shell	xmllint	$(top_srcdir)/data/io.github.GnomeMpv.appdata.xml.in \
					--xpath /component/releases/release/@version | \
				sed 's/version="\([^s]\+\)"/\1/g;s/ /\n/g' | \
				sort -rn | \
				head -1)
MESON_VERSION = $(shell $(AWK)	-F "'" \
				'/\yversion:/{print $$2}' \
				$(top_srcdir)/meson.build)

# Abort if the version numbers are not the same
dist-hook:
	if [	"$(APPDATA_VERSION)" != "$(VERSION)" -o \
		"$(MESON_VERSION)" != "$(VERSION)" ]; \
	then \
		echo	"Version numbers in appdata XML, meson.build, and" \
			"configure.ac do not match" 1>&2; \
		exit 1; \
	fi

# Test building with Meson
distcheck-hook:
	mkdir	$(PACKAGE)-$(VERSION)/_build/meson \
		$(PACKAGE)-$(VERSION)/_inst/meson && \
	cd $(PACKAGE)-$(VERSION) && \
	meson	_build/meson \
		--prefix $(abs_top_srcdir)/$(PACKAGE)-$(VERSION)/_inst/meson && \
	ninja -C _build/meson install && \
	rm -rf _build/meson _inst/meson

install-data-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)

uninstall-hook:
	$(UPDATE_DESKTOP)
	$(UPDATE_ICON)
