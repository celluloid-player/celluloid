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

#include "gmpv_mpv.h"

G_BEGIN_DECLS

#define GMPV_TYPE_PLAYER (gmpv_player_get_type())

G_DECLARE_DERIVABLE_TYPE(GmpvPlayer, gmpv_player, GMPV, PLAYER, GmpvMpv)

struct _GmpvPlayerClass
{
	GmpvMpvClass parent_class;
};

GmpvPlayer *gmpv_player_new(gint64 wid);
void gmpv_player_set_playlist_position(GmpvPlayer *player, gint64 position);
void gmpv_player_remove_playlist_entry(GmpvPlayer *player, gint64 position);
void gmpv_player_move_playlist_entry(	GmpvPlayer *player,
					gint64 src,
					gint64 dst );
void gmpv_player_set_log_level(	GmpvPlayer *player,
				const gchar *prefix,
				const gchar *level );

G_END_DECLS

#endif
