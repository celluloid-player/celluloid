AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = gnome-mpv

gnome_mpv_SOURCES =	main.c common.c mpv.c keybind.c playlist.c \
			main_window.c main_menu_bar.c control_box.c \
			pref_dialog.c open_loc_dialog.c playlist_widget.c

gnome_mpv_CFLAGS = $(GTK_CFLAGS) -DPACKAGE_LOCALEDIR=\""$(localedir)"\"
gnome_mpv_LDADD = $(GTK_LIBS)
