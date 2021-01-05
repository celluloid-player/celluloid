/*
 * Copyright (c) 2017-2020 gnome-mpv
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

#ifndef VIEW_H
#define VIEW_H

#include <glib.h>

#include "celluloid-application.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_VIEW (celluloid_view_get_type())

G_DECLARE_FINAL_TYPE(CelluloidView, celluloid_view, CELLULOID, VIEW, CelluloidMainWindow)

CelluloidView *
celluloid_view_new(CelluloidApplication *app, gboolean always_floating);

CelluloidMainWindow *
celluloid_view_get_main_window(CelluloidView *view);

void
celluloid_view_show_open_dialog(	CelluloidView *view,
					gboolean folder,
					gboolean append );

void
celluloid_view_show_open_location_dialog(CelluloidView *view, gboolean append);

void
celluloid_view_show_open_audio_track_dialog(CelluloidView *view);

void
celluloid_view_show_open_video_track_dialog(CelluloidView *view);

void
celluloid_view_show_open_subtitle_track_dialog(CelluloidView *view);

void
celluloid_view_show_save_playlist_dialog(CelluloidView *view);

void
celluloid_view_show_preferences_dialog(CelluloidView *view);

void
celluloid_view_show_shortcuts_dialog(CelluloidView *view);

void
celluloid_view_show_about_dialog(CelluloidView *view);

void
celluloid_view_show_message_dialog(	CelluloidView *view,
					GtkMessageType type,
					const gchar *title,
					const gchar *prefix,
					const gchar *msg );

void
celluloid_view_present(CelluloidView *view);

void
celluloid_view_quit(CelluloidView *view);

void
celluloid_view_reset(CelluloidView *view);

void
celluloid_view_queue_render(CelluloidView *view);

void
celluloid_view_make_gl_context_current(CelluloidView *view);

void
celluloid_view_set_use_opengl_cb(CelluloidView *view, gboolean use_opengl_cb);

gint
celluloid_view_get_scale_factor(CelluloidView *view);

void
celluloid_view_get_video_area_geometry(	CelluloidView *view,
					gint *width,
					gint *height );

void
celluloid_view_move(	CelluloidView *view,
			gboolean flip_x,
			gboolean flip_y,
			GValue *x,
			GValue *y );

void
celluloid_view_resize_video_area(CelluloidView *view, gint width, gint height);

void
celluloid_view_set_fullscreen(CelluloidView *view, gboolean fullscreen);

void
celluloid_view_set_time_position(CelluloidView *view, gdouble position);

void
celluloid_view_update_playlist(CelluloidView *view, GPtrArray *playlist);

void
celluloid_view_set_playlist_pos(CelluloidView *view, gint64 pos);

void
celluloid_view_set_playlist_visible(CelluloidView *view, gboolean visible);

gboolean
celluloid_view_get_playlist_visible(CelluloidView *view);

void
celluloid_view_set_controls_visible(CelluloidView *view, gboolean visible);

gboolean
celluloid_view_get_controls_visible(CelluloidView *view);

void
celluloid_view_set_main_menu_visible(CelluloidView *view, gboolean visible);

gboolean
celluloid_view_get_main_menu_visible(CelluloidView *view);

G_END_DECLS

#endif
