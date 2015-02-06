AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = gnome-mpv

gnome_mpv_SOURCES =	main.c def.h common.c common.h mpv.c mpv.h keybind.c \
			keybind.h playlist.c playlist.h main_window.c \
			main_window.h main_menu_bar.c main_menu_bar.h \
			control_box.c control_box.h pref_dialog.c \
			pref_dialog.h open_loc_dialog.c open_loc_dialog.h \
			playlist_widget.c playlist_widget.h

gnome_mpv_CFLAGS = $(DEPS_CFLAGS) -DPACKAGE_LOCALEDIR=\""$(localedir)"\"
gnome_mpv_LDADD = $(DEPS_LIBS)
