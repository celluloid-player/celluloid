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

#ifndef VIDEO_AREA_H
#define VIDEO_AREA_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "celluloid-control-box.h"

#define CELLULOID_TYPE_VIDEO_AREA (celluloid_video_area_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidVideoArea, celluloid_video_area, CELLULOID, VIDEO_AREA, GtkOverlay)

GtkWidget *
celluloid_video_area_new(void);

void
celluloid_video_area_update_track_list(	CelluloidVideoArea *hdr,
					const GPtrArray *track_list );

void
celluloid_video_area_set_fullscreen_state(	CelluloidVideoArea *area,
						gboolean fullscreen );

void
celluloid_video_area_set_control_box_visible(	CelluloidVideoArea *area,
						gboolean visible );

void
celluloid_video_area_set_use_opengl(	CelluloidVideoArea *area,
					gboolean use_opengl );

void
celluloid_video_area_queue_render(CelluloidVideoArea *area);

GtkDrawingArea *
celluloid_video_area_get_draw_area(CelluloidVideoArea *area);

GtkGLArea *
celluloid_video_area_get_gl_area(CelluloidVideoArea *area);

CelluloidControlBox *
celluloid_video_area_get_control_box(CelluloidVideoArea *area);

gint64
celluloid_video_area_get_xid(CelluloidVideoArea *area);

#endif
