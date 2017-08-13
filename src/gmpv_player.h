/*
 * Copyright (c) 2017 gnome-mpv
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

#ifndef PLAYER_H
#define PLAYER_H

#include <glib-object.h>

#include "gmpv_mpv.h"

G_BEGIN_DECLS

#define GMPV_TYPE_PLAYER (gmpv_player_get_type())

G_DECLARE_FINAL_TYPE(GmpvPlayer, gmpv_player, GMPV, PLAYER, GmpvMpv)

GmpvPlayer *gmpv_player_new(gint64 wid);
GPtrArray *gmpv_player_get_playlist(GmpvPlayer *player);
GPtrArray *gmpv_player_get_metadata(GmpvPlayer *player);
GPtrArray *gmpv_player_get_track_list(GmpvPlayer *player);

G_END_DECLS

#endif
