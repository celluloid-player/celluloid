AUTOMAKE_OPTIONS = foreign

AM_CFLAGS=$(GTK_CFLAGS)
AM_LDFLAGS=$(GTK_LIBS)

UPDATE_DESKTOP = update-desktop-database -q || :

bin_PROGRAMS = gnome-mpv
gnome_mpv_SOURCES = main.c main_window.c pref_dialog.c open_loc_dialog.c

desktopdir = $(datadir)/applications
desktop_DATA = gnome-mpv.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gnome-mpv.svg

install-data-hook:
	$(UPDATE_DESKTOP)

uninstall-hook:
	$(UPDATE_DESKTOP)
