AUTOMAKE_OPTIONS = foreign

UPDATE_DESKTOP = update-desktop-database -q || :

bin_PROGRAMS = gnome-mpv

gnome_mpv_SOURCES =	main.c common.c mpv.c keybind.c playlist.c \
			main_window.c main_menu_bar.c control_box.c \
			pref_dialog.c open_loc_dialog.c playlist_widget.c

gnome_mpv_CFLAGS = $(GTK_CFLAGS)
gnome_mpv_LDADD = $(GTK_LIBS)

desktopdir = $(datadir)/applications
desktop_DATA = gnome-mpv.desktop

icondir = $(datadir)/pixmaps
icon_DATA = gnome-mpv.svg

install-data-hook:
	$(UPDATE_DESKTOP)

uninstall-hook:
	$(UPDATE_DESKTOP)
