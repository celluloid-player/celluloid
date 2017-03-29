/*
 * Copyright (c) 2014, 2016-2017 gnome-mpv
 *
 * This file is part of GNOME MPV.
 *
 * GNOME MPV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNOME MPV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNOME MPV.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MPV_OBJ_H
#define MPV_OBJ_H

#include <glib.h>
#include <glib-object.h>
#include <mpv/client.h>
#include <mpv/opengl_cb.h>

#include "gmpv_playlist.h"
#include "gmpv_geometry.h"
#include "gmpv_track.h"

G_BEGIN_DECLS

#define GMPV_TYPE_MPV_OBJ (gmpv_mpv_get_type())

G_DECLARE_FINAL_TYPE(GmpvMpv, gmpv_mpv, GMPV, MPV_OBJ, GObject)

typedef struct _GmpvMpvState GmpvMpvState;
typedef struct _GmpvMetadataEntry GmpvMetadataEntry;
typedef struct _GmpvPlaylistEntry GmpvPlaylistEntry;

struct _GmpvMpvState
{
	gboolean ready;
	gboolean paused;
	gboolean loaded;
	gboolean new_file;
	gboolean init_load;
};

struct _GmpvMetadataEntry
{
	gchar *key;
	gchar *value;
};

struct _GmpvPlaylistEntry
{
	gchar *filename;
	gchar *title;
};

GmpvMetadataEntry *gmpv_metadata_entry_new(const gchar *key, const gchar *value);
void gmpv_metadata_entry_free(GmpvMetadataEntry *entry);

GmpvPlaylistEntry *gmpv_playlist_entry_new(	const gchar *filename,
						const gchar *title );
void gmpv_playlist_entry_free(GmpvPlaylistEntry *entry);

GmpvMpv *gmpv_mpv_new(GmpvPlaylist *playlist, gint64 wid);
const GmpvMpvState *gmpv_mpv_get_state(GmpvMpv *mpv);
GmpvGeometry *gmpv_mpv_get_geometry(GmpvMpv *mpv);
GmpvPlaylist *gmpv_mpv_get_playlist(GmpvMpv *mpv);
mpv_handle *gmpv_mpv_get_mpv_handle(GmpvMpv *mpv);
mpv_opengl_cb_context *gmpv_mpv_get_opengl_cb_context(GmpvMpv *mpv);
gboolean gmpv_mpv_get_use_opengl_cb(GmpvMpv *mpv);
GSList *gmpv_mpv_get_metadata(GmpvMpv *mpv);
GSList *gmpv_mpv_get_playlist_slist(GmpvMpv *mpv);
GSList *gmpv_mpv_get_track_list(GmpvMpv *mpv);
void gmpv_mpv_initialize(GmpvMpv *mpv);
void gmpv_mpv_init_gl(GmpvMpv *mpv);
void gmpv_mpv_reset(GmpvMpv *mpv);
void gmpv_mpv_quit(GmpvMpv *mpv);
void gmpv_mpv_load_track(GmpvMpv *mpv, const gchar *uri, TrackType type);
void gmpv_mpv_load_file(	GmpvMpv *mpv,
				const gchar *uri,
				gboolean append,
				gboolean update );
void gmpv_mpv_load(	GmpvMpv *mpv,
			const gchar *uri,
			gboolean append,
			gboolean update );
void gmpv_mpv_free(gpointer data);

G_END_DECLS

#endif
