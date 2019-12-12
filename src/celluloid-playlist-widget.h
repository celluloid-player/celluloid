/*
 * Copyright (c) 2014-2019 gnome-mpv
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

#ifndef PLAYLIST_WIDGET_H
#define PLAYLIST_WIDGET_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "celluloid-metadata-cache.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_PLAYLIST_WIDGET (celluloid_playlist_widget_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidPlaylistWidget, celluloid_playlist_widget, CELLULOID, PLAYLIST_WIDGET, GtkBox)

GtkWidget *
celluloid_playlist_widget_new(void);

gboolean
celluloid_playlist_widget_empty(CelluloidPlaylistWidget *wgt);

void
celluloid_playlist_widget_set_indicator_pos(	CelluloidPlaylistWidget *wgt,
						gint pos );

void
celluloid_playlist_widget_copy_selected(CelluloidPlaylistWidget *wgt);

void
celluloid_playlist_widget_remove_selected(CelluloidPlaylistWidget *wgt);

void
celluloid_playlist_widget_queue_draw(CelluloidPlaylistWidget *wgt);

void
celluloid_playlist_widget_update_contents(	CelluloidPlaylistWidget *wgt,
						GPtrArray* playlist );

GPtrArray *
celluloid_playlist_widget_get_contents(CelluloidPlaylistWidget *wgt);

G_END_DECLS

#endif
