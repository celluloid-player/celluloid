/*
 * Copyright (c) 2016-2019 gnome-mpv
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

#ifndef PLAYER_OPTIONS_H
#define PLAYER_OPTIONS_H

#include <glib.h>
#include <mpv/client.h>

#include "celluloid-player.h"
#include "celluloid-player.h"
#include "celluloid-main-window.h"

G_BEGIN_DECLS

typedef struct module_log_level module_log_level;

struct module_log_level
{
	gchar *prefix;
	mpv_log_level level;
};

void
module_log_level_free(module_log_level *level);

void
celluloid_player_options_init(	CelluloidPlayer *player,
				CelluloidMainWindow *window );

G_END_DECLS

#endif
