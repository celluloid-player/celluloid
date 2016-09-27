AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = gnome-mpv

mpris_generated = mpris/gmpv_mpris_gdbus.c mpris/gmpv_mpris_gdbus.h
mpris_files =	mpris/gmpv_mpris.c mpris/gmpv_mpris.h \
		mpris/gmpv_mpris_base.c mpris/gmpv_mpris_base.h \
		mpris/gmpv_mpris_player.c mpris/gmpv_mpris_player.h \
		$(mpris_generated)
$(mpris_generated): $(top_srcdir)/data/gmpv_mpris_gdbus.xml
	$(AM_V_GEN) \
	$(GDBUS_CODEGEN) --c-namespace gmpv_mpris --generate-c-code mpris/gmpv_mpris_gdbus $< && \
	$(SED) -i '1i#pragma GCC diagnostic ignored \"-Wconversion\"' mpris/gmpv_mpris_gdbus.c

marshal_generated = gmpv_marshal.c gmpv_marshal.h
marshal_files = $(marshal_generated)
gmpv_marshal.h: $(top_srcdir)/data/gmpv_marshal.lst
	$(AM_V_GEN) \
	$(GLIB_GENMARSHAL) --prefix="g_cclosure_gen_marshal" --header $< > $@
gmpv_marshal.c: $(top_srcdir)/data/gmpv_marshal.lst
	$(AM_V_GEN) \
	$(GLIB_GENMARSHAL) --prefix="g_cclosure_gen_marshal" --body $< > $@ && \
	$(SED) -i '1i#pragma GCC diagnostic ignored \"-Wmissing-prototypes\"' $@

authors_generated = gmpv_authors.h
authors_files = $(authors_generated)
gmpv_authors.h: $(top_srcdir)/AUTHORS
	$(AM_V_GEN) \
	$(AWK)	'BEGIN{print	"#ifndef AUTHORS_H\n" \
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
			gmpv_geometry.h \
			gmpv_header_bar.c gmpv_header_bar.h \
			gmpv_inputctl.c gmpv_inputctl.h \
			gmpv_main_window.c gmpv_main_window.h \
			gmpv_menu.c gmpv_menu.h \
			gmpv_mpv.c gmpv_mpv.h \
			gmpv_mpv_private.h \
			gmpv_mpv_opt.c gmpv_mpv_opt.h \
			gmpv_open_loc_dialog.c gmpv_open_loc_dialog.h \
			gmpv_playbackctl.c gmpv_playbackctl.h \
			gmpv_playlist.c gmpv_playlist.h \
			gmpv_playlist_widget.c gmpv_playlist_widget.h \
			gmpv_plugins_manager.c gmpv_plugins_manager.h \
			gmpv_plugins_manager_item.c gmpv_plugins_manager_item.h \
			gmpv_pref_dialog.c gmpv_pref_dialog.h \
			gmpv_seek_bar.c gmpv_seek_bar.h \
			gmpv_track.c gmpv_track.h \
			gmpv_video_area.c gmpv_video_area.h \
			$(mpris_files) $(marshal_files) $(media_key_files) \
			$(authors_files)

if NEW_GTK
gnome_mpv_SOURCES += gmpv_shortcuts_window.c gmpv_shortcuts_window.h
endif

gnome_mpv_CFLAGS =	$(DEPS_CFLAGS) \
			-DG_SETTINGS_ENABLE_BACKEND \
			-DPACKAGE_LOCALEDIR=\""$(localedir)"\"

gnome_mpv_LDADD = $(DEPS_LIBS)

CLEANFILES = $(mpris_generated) $(marshal_generated) $(authors_generated)
