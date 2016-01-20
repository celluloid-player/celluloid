AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = gnome-mpv

mpris_generated = mpris/mpris_gdbus.c mpris/mpris_gdbus.h

$(mpris_generated): $(top_srcdir)/data/mpris_gdbus.xml
	gdbus-codegen --c-namespace mpris --generate-c-code mpris/mpris_gdbus $< && \
	sed -i '1i#pragma GCC diagnostic ignored \"-Wconversion\"' mpris/mpris_gdbus.c

mpris_files =	mpris/mpris.c mpris/mpris.h mpris/mpris_base.c \
		mpris/mpris_base.h mpris/mpris_player.c mpris/mpris_player.h \
		$(mpris_generated)

media_key_files = media_keys/media_keys.c media_keys/media_keys.h

BUILT_SOURCES = $(mpris_generated)

gnome_mpv_SOURCES =	main.c def.h \
			actionctl.c actionctl.h \
			common.c common.h \
			control_box.c control_box.h \
			keybind.c keybind.h \
			main_window.c main_window.h \
			menu.c menu.h \
			mpv.c mpv.h \
			open_loc_dialog.c open_loc_dialog.h \
			playbackctl.c playbackctl.h \
			playlist.c playlist.h \
			playlist_widget.c playlist_widget.h \
			pref_dialog.c pref_dialog.h \
			track.c track.h \
			$(mpris_files) $(media_key_files)

gnome_mpv_CFLAGS =	$(DEPS_CFLAGS) \
			-DG_SETTINGS_ENABLE_BACKEND \
			-DPACKAGE_LOCALEDIR=\""$(localedir)"\"

gnome_mpv_LDADD = $(DEPS_LIBS)

CLEANFILES = $(mpris_generated)
