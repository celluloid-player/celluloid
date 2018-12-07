/*
 * Copyright (c) 2014-2017 gnome-mpv
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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gmpv_playlist_widget.h"
#include "gmpv_control_box.h"
#include "gmpv_video_area.h"

G_BEGIN_DECLS

#define GMPV_TYPE_MAIN_WINDOW (gmpv_main_window_get_type ())

G_DECLARE_FINAL_TYPE(GmpvMainWindow, gmpv_main_window, GMPV, MAIN_WINDOW, GtkApplicationWindow)

GtkWidget *gmpv_main_window_new(	GtkApplication *app,
					gboolean always_floating );
GmpvPlaylistWidget *gmpv_main_window_get_playlist(GmpvMainWindow *wnd);
GmpvControlBox *gmpv_main_window_get_control_box(GmpvMainWindow *wnd);
GmpvVideoArea *gmpv_main_window_get_video_area(GmpvMainWindow *wnd);
void gmpv_main_window_set_use_floating_controls(	GmpvMainWindow *wnd,
							gboolean floating );
gboolean gmpv_main_window_get_use_floating_controls(GmpvMainWindow *wnd);
void gmpv_main_window_set_fullscreen(GmpvMainWindow *wnd, gboolean fullscreen);
gboolean gmpv_main_window_get_fullscreen(GmpvMainWindow *wnd);
void gmpv_main_window_toggle_fullscreen(GmpvMainWindow *wnd);
void gmpv_main_window_reset(GmpvMainWindow *wnd);
void gmpv_main_window_save_state(GmpvMainWindow *wnd);
void gmpv_main_window_load_state(GmpvMainWindow *wnd);
void gmpv_main_window_update_track_list(	GmpvMainWindow *wnd,
						const GPtrArray *track_list );
void gmpv_main_window_resize_video_area(	GmpvMainWindow *wnd,
						gint width,
						gint height );
void gmpv_main_window_enable_csd(GmpvMainWindow *wnd);
gboolean gmpv_main_window_get_csd_enabled(GmpvMainWindow *wnd);
void gmpv_main_window_set_playlist_visible(	GmpvMainWindow *wnd,
						gboolean visible );
gboolean gmpv_main_window_get_playlist_visible(GmpvMainWindow *wnd);
void gmpv_main_window_set_controls_visible(	GmpvMainWindow *wnd,
						gboolean visible );
gboolean gmpv_main_window_get_controls_visible(GmpvMainWindow *wnd);

G_END_DECLS

#endif
