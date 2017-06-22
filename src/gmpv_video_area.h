/*
 * Copyright (c) 2016-2017 gnome-mpv
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

#ifndef VIDEO_AREA_H
#define VIDEO_AREA_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define GMPV_TYPE_VIDEO_AREA (gmpv_video_area_get_type ())

G_DECLARE_FINAL_TYPE(GmpvVideoArea, gmpv_video_area, GMPV, VIDEO_AREA, GtkOverlay)

GtkWidget *gmpv_video_area_new(void);
void gmpv_video_area_update_track_list(	GmpvVideoArea *hdr,
					const GPtrArray *track_list );
void gmpv_video_area_set_fullscreen_state(	GmpvVideoArea *area,
						gboolean fullscreen );
void gmpv_video_area_set_control_box(	GmpvVideoArea *area,
					GtkWidget *control_box );
void gmpv_video_area_set_use_opengl(GmpvVideoArea *area, gboolean use_opengl);
void gmpv_video_area_queue_render(GmpvVideoArea *area);
GtkDrawingArea *gmpv_video_area_get_draw_area(GmpvVideoArea *area);
GtkGLArea *gmpv_video_area_get_gl_area(GmpvVideoArea *area);
gint64 gmpv_video_area_get_xid(GmpvVideoArea *area);

#endif
