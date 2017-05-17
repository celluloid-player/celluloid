/*
 * Copyright (c) 2014-2017 gnome-mpv
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

#ifndef COMMON_H
#define COMMON_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GmpvPlaylistEntry GmpvPlaylistEntry;
typedef struct _GmpvMetadataEntry GmpvMetadataEntry;

typedef enum TrackType TrackType;
typedef struct GmpvTrack GmpvTrack;

enum TrackType
{
	TRACK_TYPE_INVALID,
	TRACK_TYPE_AUDIO,
	TRACK_TYPE_VIDEO,
	TRACK_TYPE_SUBTITLE,
	TRACK_TYPE_N
};

struct _GmpvPlaylistEntry
{
	gchar *filename;
	gchar *title;
};

struct _GmpvMetadataEntry
{
	gchar *key;
	gchar *value;
};

struct GmpvTrack
{
	TrackType type;
	gint64 id;
	gchar *title;
	gchar *lang;
};

GmpvPlaylistEntry *gmpv_playlist_entry_new(	const gchar *filename,
						const gchar *title );
void gmpv_playlist_entry_free(GmpvPlaylistEntry *entry);

GmpvMetadataEntry *gmpv_metadata_entry_new(	const gchar *key,
						const gchar *value );
void gmpv_metadata_entry_free(GmpvMetadataEntry *entry);

GmpvTrack *gmpv_track_new(void);
void gmpv_track_free(GmpvTrack *entry);

gchar *get_config_dir_path(void);
gchar *get_scripts_dir_path(void);
gchar *get_path_from_uri(const gchar *uri);
gchar *get_name_from_path(const gchar *path);
gboolean extension_matches(const gchar *filename, const gchar **extensions);
void *gslist_to_array(GSList *slist);
gchar *strnjoinv(const gchar *separator, const gchar **str_array, gsize count);

G_END_DECLS

#endif
