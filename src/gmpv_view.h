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

#ifndef VIEW_H
#define VIEW_H

#include <glib.h>

#include "gmpv_application.h"

G_BEGIN_DECLS

#define GMPV_TYPE_VIEW (gmpv_view_get_type())

G_DECLARE_FINAL_TYPE(GmpvView, gmpv_view, GMPV, VIEW, GObject)

GmpvView *gmpv_view_new(GmpvApplication *app, gboolean always_floating);
GmpvMainWindow *gmpv_view_get_main_window(GmpvView *view);
void gmpv_view_show_open_dialog(GmpvView *view, gboolean append);
void gmpv_view_show_open_location_dialog(GmpvView *view, gboolean append);
void gmpv_view_show_open_audio_track_dialog(GmpvView *view);
void gmpv_view_show_open_subtitle_track_dialog(GmpvView *view);
void gmpv_view_show_save_playlist_dialog(GmpvView *view);
void gmpv_view_show_preferences_dialog(GmpvView *view);
void gmpv_view_show_shortcuts_dialog(GmpvView *view);
void gmpv_view_show_about_dialog(GmpvView *view);
void gmpv_view_show_message_dialog(	GmpvView *view,
					GtkMessageType type,
					const gchar *title,
					const gchar *prefix,
					const gchar *msg );
void gmpv_view_present(GmpvView *view);
void gmpv_view_quit(GmpvView *view);
void gmpv_view_reset(GmpvView *view);
void gmpv_view_queue_render(GmpvView *view);
void gmpv_view_make_gl_context_current(GmpvView *view);
void gmpv_view_set_use_opengl_cb(GmpvView *view, gboolean use_opengl_cb);
gint gmpv_view_get_scale_factor(GmpvView *view);
void gmpv_view_get_video_area_geometry(GmpvView *view, gint *width, gint *height);
void gmpv_view_move(	GmpvView *view,
			gboolean flip_x,
			gboolean flip_y,
			GValue *x,
			GValue *y );
void gmpv_view_resize_video_area(GmpvView *view, gint width, gint height);
void gmpv_view_set_fullscreen(GmpvView *view, gboolean fullscreen);
void gmpv_view_set_time_position(GmpvView *view, gdouble position);
void gmpv_view_update_playlist(GmpvView *view, GPtrArray *playlist);
void gmpv_view_set_playlist_pos(GmpvView *view, gint64 pos);
void gmpv_view_set_playlist_visible(GmpvView *view, gboolean visible);
gboolean gmpv_view_get_playlist_visible(GmpvView *view);
void gmpv_view_set_controls_visible(GmpvView *view, gboolean visible);
gboolean gmpv_view_get_controls_visible(GmpvView *view);

G_END_DECLS

#endif
