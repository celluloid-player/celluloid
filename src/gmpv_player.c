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

#include "gmpv_player.h"

struct _GmpvPlayer
{
	GmpvMpv parent;
};

struct _GmpvPlayerClass
{
	GmpvMpvClass parent_class;
};

G_DEFINE_TYPE(GmpvPlayer, gmpv_player, GMPV_TYPE_MPV)

static void gmpv_player_class_init(GmpvPlayerClass *klass)
{
}

static void gmpv_player_init(GmpvPlayer *player)
{
}

GmpvPlayer *gmpv_player_new(gint64 wid)
{
	return GMPV_PLAYER(g_object_new(	gmpv_player_get_type(),
						"wid", wid,
						NULL ));
}

