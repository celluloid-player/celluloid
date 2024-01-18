/*
 * Copyright (c) 2014-2019, 2022 gnome-mpv
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

#include "celluloid-playlist-widget.h"
#include "celluloid-control-box.h"
#include "celluloid-video-area.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_MAIN_WINDOW (celluloid_main_window_get_type ())

G_DECLARE_DERIVABLE_TYPE(CelluloidMainWindow, celluloid_main_window, CELLULOID, MAIN_WINDOW, GtkApplicationWindow)

struct _CelluloidMainWindowClass
{
	GtkApplicationWindowClass parent_class;
};

GtkWidget *
celluloid_main_window_new(GtkApplication *app, gboolean always_floating);

CelluloidPlaylistWidget *
celluloid_main_window_get_playlist(CelluloidMainWindow *wnd);

CelluloidControlBox *
celluloid_main_window_get_control_box(CelluloidMainWindow *wnd);

CelluloidVideoArea *
celluloid_main_window_get_video_area(CelluloidMainWindow *wnd);

void
celluloid_main_window_set_use_floating_controls(	CelluloidMainWindow *wnd,
							gboolean floating );

gboolean
celluloid_main_window_get_use_floating_controls(CelluloidMainWindow *wnd);

void
celluloid_main_window_set_use_floating_headerbar(CelluloidMainWindow *wnd,
                                                 gboolean             floating);

gboolean
celluloid_main_window_get_use_floating_headerbar(CelluloidMainWindow *wnd);

gboolean
celluloid_main_window_get_fullscreen(CelluloidMainWindow *wnd);

void
celluloid_main_window_toggle_fullscreen(CelluloidMainWindow *wnd);

void
celluloid_main_window_reset(CelluloidMainWindow *wnd);

void
celluloid_main_window_save_state(CelluloidMainWindow *wnd);

void
celluloid_main_window_load_state(CelluloidMainWindow *wnd);

void
celluloid_main_window_update_track_list(	CelluloidMainWindow *wnd,
						const GPtrArray *track_list );

void
celluloid_main_window_update_disc_list(	CelluloidMainWindow *wnd,
					const GPtrArray *disc_list );

void celluloid_main_window_resize_video_area(	CelluloidMainWindow *wnd,
						gint width,
						gint height );

void
celluloid_main_window_enable_csd(CelluloidMainWindow *wnd);

gboolean
celluloid_main_window_get_csd_enabled(CelluloidMainWindow *wnd);

void
celluloid_main_window_set_playlist_visible(	CelluloidMainWindow *wnd,
						gboolean visible );

gboolean
celluloid_main_window_get_playlist_visible(CelluloidMainWindow *wnd);

void
celluloid_main_window_set_controls_visible(	CelluloidMainWindow *wnd,
						gboolean visible );

gboolean
celluloid_main_window_get_controls_visible(CelluloidMainWindow *wnd);

G_END_DECLS

#endif
