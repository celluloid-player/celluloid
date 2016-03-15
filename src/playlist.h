/*
 * Copyright (c) 2014-2016 gnome-mpv
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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PLAYLIST_TYPE (playlist_get_type ())

#define	PLAYLIST(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
		((obj), PLAYLIST_TYPE, Playlist))

#define	PLAYLIST_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST \
		((klass), PLAYLIST_TYPE, PlaylistClass))

#define	IS_PLAYLIST(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), PLAYLIST_TYPE))

#define	IS_PLAYLIST_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), PLAYLIST_TYPE))

enum PlaylistStoreColumn
{
	PLAYLIST_NAME_COLUMN,
	PLAYLIST_URI_COLUMN,
	PLAYLIST_WEIGHT_COLUMN,
	PLAYLIST_N_COLUMNS
};

typedef enum PlaylistStoreColumn PlaylistStoreColumn;
typedef struct _Playlist Playlist;
typedef struct _PlaylistClass PlaylistClass;

Playlist *playlist_new(void);
GType playlist_get_type(void);
GtkListStore *playlist_get_store(Playlist *pl);
void playlist_set_indicator_pos(Playlist *pl, gint pos);
void playlist_append(Playlist *pl, const gchar *name, const gchar *uri);
void playlist_remove(Playlist *pl, gint pos);
void playlist_clear(Playlist *pl);
gboolean playlist_empty(Playlist *pl);
void playlist_reset(Playlist *pl);

G_END_DECLS

#endif
