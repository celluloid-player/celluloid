AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = gnome-mpv

mpris_generated = mpris/mpris_gdbus.c mpris/mpris_gdbus.h
mpris_files =	mpris/mpris.c mpris/mpris.h mpris/mpris_base.c \
		mpris/mpris_base.h mpris/mpris_player.c mpris/mpris_player.h \
		$(mpris_generated)
$(mpris_generated): $(top_srcdir)/data/mpris_gdbus.xml
	$(GDBUS_CODEGEN) --c-namespace mpris --generate-c-code mpris/mpris_gdbus $< && \
	sed -i '1i#pragma GCC diagnostic ignored \"-Wconversion\"' mpris/mpris_gdbus.c

marshal_generated = marshal.c marshal.h
marshal_files = $(marshal_generated)
marshal.h: $(top_srcdir)/data/marshal.lst
	$(GLIB_GENMARSHAL) --prefix="g_cclosure_gen_marshal" --header $< > $@
marshal.c: $(top_srcdir)/data/marshal.lst
	$(GLIB_GENMARSHAL) --prefix="g_cclosure_gen_marshal" --body $< > $@
	sed -i '1i#pragma GCC diagnostic ignored \"-Wmissing-prototypes\"' $@

authors_generated = authors.h
authors_files = $(authors_generated)
authors.h: $(top_srcdir)/AUTHORS
	awk	'BEGIN{print	"#ifndef AUTHORS_H\n" \
				"#define AUTHORS_H\n\n" \
				"#define AUTHORS {\\"} \
		/^[^#].+/{print "\"" $$0 "\",\\"} \
		END{print "NULL}\n\n#endif"}' $< > $@

media_key_files = media_keys/media_keys.c media_keys/media_keys.h

BUILT_SOURCES = $(mpris_generated) $(marshal_generated) $(authors_generated)

gnome_mpv_SOURCES =	main.c def.h \
			actionctl.c actionctl.h \
			application.c application.h \
			common.c common.h \
			control_box.c control_box.h \
			main_window.c main_window.h \
			menu.c menu.h \
			mpv_obj.c mpv_obj.h \
			mpv_opt.c mpv_opt.h \
			open_loc_dialog.c open_loc_dialog.h \
			playbackctl.c playbackctl.h \
			playlist.c playlist.h \
			playlist_widget.c playlist_widget.h \
			plugins_manager.c plugins_manager.h \
			plugins_manager_item.c plugins_manager_item.h \
			pref_dialog.c pref_dialog.h \
			shortcuts_window.c shortcuts_window.h \
			track.c track.h \
			video_area.c video_area.h \
			$(mpris_files) $(marshal_files) $(media_key_files) \
			$(authors_files)

gnome_mpv_CFLAGS =	$(DEPS_CFLAGS) \
			-DG_SETTINGS_ENABLE_BACKEND \
			-DPACKAGE_LOCALEDIR=\""$(localedir)"\"

gnome_mpv_LDADD = $(DEPS_LIBS)

CLEANFILES = $(mpris_generated) $(marshal_generated) $(authors_generated)
