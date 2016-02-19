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

#include "common.h"

#include <gtk/gtk.h>

enum PlaylistStoreColumn
{
	PLAYLIST_INDICATOR_COLUMN,
	PLAYLIST_NAME_COLUMN,
	PLAYLIST_URI_COLUMN,
	PLAYLIST_N_COLUMNS
};

typedef enum PlaylistStoreColumn PlaylistStoreColumn;
typedef GtkListStore playlist;

playlist *playlist_new(void);
void playlist_set_indicator_pos(playlist *pl, gint pos);
void playlist_append(	playlist *pl,
			const gchar *name,
			const gchar *uri );
void playlist_remove(playlist *pl, gint pos);
void playlist_clear(playlist *pl);
gboolean playlist_empty(playlist *pl);
void playlist_reset(playlist *pl);

#endif
