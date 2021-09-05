/*
 * Copyright (c) 2021 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYLIST_ITEM_H
#define PLAYLIST_ITEM_H

#include <gio/gio.h>

#define CELLULOID_TYPE_PLAYLIST_ITEM (celluloid_playlist_item_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidPlaylistItem, celluloid_playlist_item, CELLULOID, PLAYLIST_ITEM, GObject)

CelluloidPlaylistItem *
celluloid_playlist_item_new_take(	gchar *title,
					gchar *uri,
					gboolean is_current );

CelluloidPlaylistItem *
celluloid_playlist_item_new(	gchar *title,
				gchar *uri,
				gboolean is_current );

CelluloidPlaylistItem *
celluloid_playlist_item_copy(CelluloidPlaylistItem *source);

const gchar *
celluloid_playlist_item_get_title(CelluloidPlaylistItem *self);

const gchar *
celluloid_playlist_item_get_uri(CelluloidPlaylistItem *self);

gboolean
celluloid_playlist_item_get_is_current(CelluloidPlaylistItem *self);

void
celluloid_playlist_item_set_is_current(	CelluloidPlaylistItem *self,
					gboolean value );

#endif
