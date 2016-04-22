/*
 * Copyright (c) 2016 gnome-mpv
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

#include <gtk/gtk.h>

#define VIDEO_AREA_TYPE (video_area_get_type ())

#define	VIDEO_AREA(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), VIDEO_AREA_TYPE, VideoArea))

#define	VIDEO_AREA_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), VIDEO_AREA_TYPE, VideoAreaClass))

#define	IS_VIDEO_AREA(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), VIDEO_AREA_TYPE))

#define	IS_VIDEO_AREA_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), VIDEO_AREA_TYPE))

typedef struct _VideoArea VideoArea;
typedef struct _VideoAreaClass VideoAreaClass;

GtkWidget *video_area_new(void);
GType video_area_get_type(void);
void video_area_set_fullscreen_state(VideoArea *area, gboolean fullscreen);
void video_area_set_control_box(VideoArea *area, GtkWidget *control_box);
void video_area_set_use_opengl(VideoArea *area, gboolean use_opengl);
GtkDrawingArea *video_area_get_draw_area(VideoArea *area);
GtkGLArea *video_area_get_gl_area(VideoArea *area);
gint64 video_area_get_xid(VideoArea *area);

#endif
