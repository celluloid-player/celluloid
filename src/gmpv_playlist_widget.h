/*
 * Copyright (c) 2014-2017 gnome-mpv
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

#ifndef PLAYLIST_WIDGET_H
#define PLAYLIST_WIDGET_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GMPV_TYPE_PLAYLIST_WIDGET (gmpv_playlist_widget_get_type ())

G_DECLARE_FINAL_TYPE(GmpvPlaylistWidget, gmpv_playlist_widget, GMPV, PLAYLIST_WIDGET, GtkScrolledWindow)

GtkWidget *gmpv_playlist_widget_new(void);
gboolean gmpv_playlist_widget_empty(GmpvPlaylistWidget *wgt);
void gmpv_playlist_widget_set_indicator_pos(GmpvPlaylistWidget *wgt, gint pos);
void gmpv_playlist_widget_remove_selected(GmpvPlaylistWidget *wgt);
void gmpv_playlist_widget_queue_draw(GmpvPlaylistWidget *wgt);
void gmpv_playlist_widget_update_contents(	GmpvPlaylistWidget *wgt,
						GPtrArray* playlist );
GPtrArray *gmpv_playlist_widget_get_contents(GmpvPlaylistWidget *wgt);

G_END_DECLS

#endif
