/*
 * Copyright (c) 2017-2019 gnome-mpv
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

#ifndef PLAYER_H
#define PLAYER_H

#include <glib-object.h>

#include "celluloid-mpv.h"
#include "celluloid-mpv.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_PLAYER (celluloid_player_get_type())

G_DECLARE_DERIVABLE_TYPE(CelluloidPlayer, celluloid_player, CELLULOID, PLAYER, CelluloidMpv)

struct _CelluloidPlayerClass
{
	CelluloidMpvClass parent_class;
	void (*autofit)(CelluloidPlayer *player);
	void (*metadata_update)(CelluloidPlayer *player, gint64 pos);
};

CelluloidPlayer *
celluloid_player_new(gint64 wid);

void
celluloid_player_set_playlist_position(CelluloidPlayer *player, gint64 position);

void
celluloid_player_remove_playlist_entry(CelluloidPlayer *player, gint64 position);

void
celluloid_player_move_playlist_entry(	CelluloidPlayer *player,
					gint64 src,
					gint64 dst );

void
celluloid_player_set_log_level(	CelluloidPlayer *player,
				const gchar *prefix,
				const gchar *level );

G_END_DECLS

#endif
