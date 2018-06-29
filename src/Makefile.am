AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = gnome-mpv

mpris_generated = mpris/gmpv_mpris_gdbus.c mpris/gmpv_mpris_gdbus.h
mpris_files =	mpris/gmpv_mpris.c mpris/gmpv_mpris.h \
		mpris/gmpv_mpris_module.c mpris/gmpv_mpris_module.h \
		mpris/gmpv_mpris_base.c mpris/gmpv_mpris_base.h \
		mpris/gmpv_mpris_player.c mpris/gmpv_mpris_player.h \
		mpris/gmpv_mpris_track_list.c mpris/gmpv_mpris_track_list.h \
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
			gmpv_application.c gmpv_application.h \
			gmpv_common.c gmpv_common.h \
			gmpv_controller.c gmpv_controller.h \
			gmpv_controller_actions.c gmpv_controller_actions.h \
			gmpv_controller_private.h \
			gmpv_controller_input.c gmpv_controller_input.h \
			gmpv_control_box.c gmpv_control_box.h \
			gmpv_file_chooser.c gmpv_file_chooser.h \
			gmpv_header_bar.c gmpv_header_bar.h \
			gmpv_main_window.c gmpv_main_window.h \
			gmpv_menu.c gmpv_menu.h \
			gmpv_metadata_cache.c gmpv_metadata_cache.h \
			gmpv_model.c gmpv_model.h \
			gmpv_mpv.c gmpv_mpv.h \
			gmpv_mpv_private.h \
			gmpv_open_location_dialog.c gmpv_open_location_dialog.h \
			gmpv_player.c gmpv_player.h \
			gmpv_player_options.c gmpv_player_options.h \
			gmpv_playlist_widget.c gmpv_playlist_widget.h \
			gmpv_plugins_manager.c gmpv_plugins_manager.h \
			gmpv_plugins_manager_item.c gmpv_plugins_manager_item.h \
			gmpv_preferences_dialog.c gmpv_preferences_dialog.h \
			gmpv_seek_bar.c gmpv_seek_bar.h \
			gmpv_shortcuts_window.c gmpv_shortcuts_window.h \
			gmpv_video_area.c gmpv_video_area.h \
			gmpv_view.c gmpv_view.h \
			gmpv_mpv_wrapper.c gmpv_mpv_wrapper.h \
			$(mpris_files) $(marshal_files) $(media_key_files) \
			$(authors_files)

gnome_mpv_CFLAGS =	$(DEPS_CFLAGS) \
			-DG_SETTINGS_ENABLE_BACKEND \
			-DPACKAGE_LOCALEDIR=\""$(localedir)"\" \
			-Impris

gnome_mpv_LDADD = $(DEPS_LIBS)

CLEANFILES = $(mpris_generated) $(marshal_generated) $(authors_generated)
EXTRA_DIST = meson.build generate_authors.py
