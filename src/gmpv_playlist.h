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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GMPV_TYPE_PLAYLIST (gmpv_playlist_get_type ())

G_DECLARE_FINAL_TYPE(GmpvPlaylist, gmpv_playlist, GMPV, PLAYLIST, GObject)

enum PlaylistStoreColumn
{
	PLAYLIST_NAME_COLUMN,
	PLAYLIST_URI_COLUMN,
	PLAYLIST_WEIGHT_COLUMN,
	PLAYLIST_N_COLUMNS
};

typedef enum PlaylistStoreColumn PlaylistStoreColumn;

GmpvPlaylist *gmpv_playlist_new(void);
GtkListStore *gmpv_playlist_get_store(GmpvPlaylist *pl);
void gmpv_playlist_set_indicator_pos(GmpvPlaylist *pl, gint pos);
void gmpv_playlist_append(GmpvPlaylist *pl, const gchar *name, const gchar *uri);
void gmpv_playlist_remove(GmpvPlaylist *pl, gint pos);
void gmpv_playlist_clear(GmpvPlaylist *pl);
gboolean gmpv_playlist_empty(GmpvPlaylist *pl);
void gmpv_playlist_reset(GmpvPlaylist *pl);

G_END_DECLS

#endif
