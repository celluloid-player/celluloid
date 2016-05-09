AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = gnome-mpv

mpris_generated = mpris/gmpv_mpris_gdbus.c mpris/gmpv_mpris_gdbus.h
mpris_files =	mpris/gmpv_mpris.c mpris/gmpv_mpris.h \
		mpris/gmpv_mpris_base.c mpris/gmpv_mpris_base.h \
		mpris/gmpv_mpris_player.c mpris/gmpv_mpris_player.h \
		$(mpris_generated)
$(mpris_generated): $(top_srcdir)/data/gmpv_mpris_gdbus.xml
	$(GDBUS_CODEGEN) --c-namespace gmpv_mpris --generate-c-code mpris/gmpv_mpris_gdbus $< && \
	sed -i '1i#pragma GCC diagnostic ignored \"-Wconversion\"' mpris/gmpv_mpris_gdbus.c

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

media_key_files = media_keys/gmpv_media_keys.c media_keys/gmpv_media_keys.h

BUILT_SOURCES = $(mpris_generated) $(marshal_generated) $(authors_generated)

gnome_mpv_SOURCES =	gmpv_main.c gmpv_def.h \
			gmpv_actionctl.c gmpv_actionctl.h \
			gmpv_application.c gmpv_application.h \
			gmpv_common.c gmpv_common.h \
			gmpv_control_box.c gmpv_control_box.h \
			gmpv_main_window.c gmpv_main_window.h \
			gmpv_menu.c gmpv_menu.h \
			gmpv_mpv_obj.c gmpv_mpv_obj.h \
			gmpv_mpv_obj_private.h \
			gmpv_mpv_opt.c gmpv_mpv_opt.h \
			gmpv_open_loc_dialog.c gmpv_open_loc_dialog.h \
			gmpv_playbackctl.c gmpv_playbackctl.h \
			gmpv_playlist.c gmpv_playlist.h \
			gmpv_playlist_widget.c gmpv_playlist_widget.h \
			gmpv_plugins_manager.c gmpv_plugins_manager.h \
			gmpv_plugins_manager_item.c gmpv_plugins_manager_item.h \
			gmpv_pref_dialog.c gmpv_pref_dialog.h \
			gmpv_shortcuts_window.c gmpv_shortcuts_window.h \
			gmpv_track.c gmpv_track.h \
			gmpv_video_area.c gmpv_video_area.h \
			$(mpris_files) $(marshal_files) $(media_key_files) \
			$(authors_files)

gnome_mpv_CFLAGS =	$(DEPS_CFLAGS) \
			-DG_SETTINGS_ENABLE_BACKEND \
			-DPACKAGE_LOCALEDIR=\""$(localedir)"\"

gnome_mpv_LDADD = $(DEPS_LIBS)

CLEANFILES = $(mpris_generated) $(marshal_generated) $(authors_generated)
