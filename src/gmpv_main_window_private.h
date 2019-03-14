/*
 * Copyright (c) 2019 gnome-mpv
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

#ifndef GMPV_MAIN_WINDOW_PRIVATE_H
#define GMPV_MAIN_WINDOW_PRIVATE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GmpvMainWindowPrivate GmpvMainWindowPrivate;

enum
{
	PROP_0,
	PROP_ALWAYS_FLOATING,
	N_PROPERTIES
};

struct _GmpvMainWindowPrivate
{
	GtkApplicationWindow parent;
	gint width_offset;
	gint height_offset;
	gint resize_target[2];
	gboolean csd;
	gboolean always_floating;
	gboolean use_floating_controls;
	gboolean fullscreen;
	gboolean playlist_visible;
	gboolean playlist_first_toggle;
	gboolean pre_fs_playlist_visible;
	gint playlist_width;
	guint resize_tag;
	const GPtrArray *track_list;
	GtkWidget *header_bar;
	GtkWidget *main_box;
	GtkWidget *vid_area_paned;
	GtkWidget *vid_area;
	GtkWidget *control_box;
	GtkWidget *playlist;
};

#define get_private(window) \
	G_TYPE_INSTANCE_GET_PRIVATE(window, GMPV_TYPE_MAIN_WINDOW, GmpvMainWindowPrivate)

G_END_DECLS

#endif

