AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = celluloid

mpris_generated = mpris/celluloid-mpris-gdbus.c mpris/celluloid-mpris-gdbus.h
mpris_files =	mpris/celluloid-mpris.c mpris/celluloid-mpris.h \
		mpris/celluloid-mpris-module.c mpris/celluloid-mpris-module.h \
		mpris/celluloid-mpris-base.c mpris/celluloid-mpris-base.h \
		mpris/celluloid-mpris-player.c mpris/celluloid-mpris-player.h \
		mpris/celluloid-mpris-track-list.c mpris/celluloid-mpris-track-list.h \
		$(mpris_generated)
$(mpris_generated): $(top_srcdir)/data/celluloid-mpris-gdbus.xml
	$(AM_V_GEN) \
	$(GDBUS_CODEGEN) --c-namespace celluloid_mpris --generate-c-code mpris/celluloid-mpris-gdbus $< && \
	$(SED) -i '1i#pragma GCC diagnostic ignored \"-Wconversion\"' mpris/celluloid-mpris-gdbus.c

marshal_generated = celluloid-marshal.c celluloid-marshal.h
marshal_files = $(marshal_generated)
celluloid-marshal.h: $(top_srcdir)/data/celluloid-marshal.lst
	$(AM_V_GEN) \
	$(GLIB_GENMARSHAL) --prefix="g_cclosure_gen_marshal" --header $< > $@
celluloid-marshal.c: $(top_srcdir)/data/celluloid-marshal.lst
	$(AM_V_GEN) \
	$(GLIB_GENMARSHAL) --prefix="g_cclosure_gen_marshal" --body $< > $@ && \
	$(SED) -i '1i#pragma GCC diagnostic ignored \"-Wmissing-prototypes\"' $@

authors_generated = celluloid-authors.h
authors_files = $(authors_generated)
celluloid-authors.h: $(top_srcdir)/AUTHORS
	$(AM_V_GEN) \
	$(AWK)	'BEGIN{print	"#ifndef AUTHORS_H\n" \
				"#define AUTHORS_H\n\n" \
				"#define AUTHORS {\\"} \
		/^[^#].+/{print "\"" $$0 "\",\\"} \
		END{print "NULL}\n\n#endif"}' $< > $@

media_key_files = media-keys/celluloid-media-keys.c media-keys/celluloid-media-keys.h

BUILT_SOURCES = $(mpris_generated) $(marshal_generated) $(authors_generated)

celluloid_SOURCES =	celluloid-main.c celluloid-def.h \
			celluloid-application.c celluloid-application.h \
			celluloid-common.c celluloid-common.h \
			celluloid-controller.c celluloid-controller.h \
			celluloid-controller-actions.c celluloid-controller-actions.h \
			celluloid-controller-private.h \
			celluloid-controller-input.c celluloid-controller-input.h \
			celluloid-control-box.c celluloid-control-box.h \
			celluloid-file-chooser.c celluloid-file-chooser.h \
			celluloid-header-bar.c celluloid-header-bar.h \
			celluloid-main-window.c celluloid-main-window.h celluloid-main-window-private.h \
			celluloid-menu.c celluloid-menu.h \
			celluloid-metadata-cache.c celluloid-metadata-cache.h \
			celluloid-model.c celluloid-model.h \
			celluloid-mpv.c celluloid-mpv.h \
			celluloid-mpv-private.h \
			celluloid-open-location-dialog.c celluloid-open-location-dialog.h \
			celluloid-player.c celluloid-player.h celluloid-player-private.h \
			celluloid-player-options.c celluloid-player-options.h \
			celluloid-playlist-widget.c celluloid-playlist-widget.h \
			celluloid-plugins-manager.c celluloid-plugins-manager.h \
			celluloid-plugins-manager-item.c celluloid-plugins-manager-item.h \
			celluloid-preferences-dialog.c celluloid-preferences-dialog.h \
			celluloid-seek-bar.c celluloid-seek-bar.h \
			celluloid-shortcuts-window.c celluloid-shortcuts-window.h \
			celluloid-video-area.c celluloid-video-area.h \
			celluloid-view.c celluloid-view.h \
			celluloid-mpv-wrapper.c celluloid-mpv-wrapper.h \
			$(mpris_files) $(marshal_files) $(media_key_files) \
			$(authors_files)

celluloid_CFLAGS =	$(DEPS_CFLAGS) \
			-DG_SETTINGS_ENABLE_BACKEND \
			-DPACKAGE_LOCALEDIR=\""$(localedir)"\" \
			-Impris

celluloid_LDADD = $(DEPS_LIBS)

CLEANFILES = $(mpris_generated) $(marshal_generated) $(authors_generated)
EXTRA_DIST = meson.build generate-authors.py
