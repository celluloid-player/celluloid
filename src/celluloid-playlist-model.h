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

#ifndef PLAYLIST_MODEL_H
#define PLAYLIST_MODEL_H

#include "celluloid-playlist-item.h"

#define CELLULOID_TYPE_PLAYLIST_MODEL (celluloid_playlist_model_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidPlaylistModel, celluloid_playlist_model, CELLULOID, PLAYLIST_MODEL, GObject)

CelluloidPlaylistModel *
celluloid_playlist_model_new(void);

void
celluloid_playlist_model_append(	CelluloidPlaylistModel *self,
					CelluloidPlaylistItem *item );

void
celluloid_playlist_model_remove(CelluloidPlaylistModel *self, guint position);

void
celluloid_playlist_model_clear(CelluloidPlaylistModel *self);

gint
celluloid_playlist_model_get_current(CelluloidPlaylistModel *self);

void
celluloid_playlist_model_set_current(	CelluloidPlaylistModel *self,
					gint position);

#endif
