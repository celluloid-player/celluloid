/*
 * Copyright (c) 2016-2021 gnome-mpv
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

#ifndef HEADER_BAR_H
#define HEADER_BAR_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define CELLULOID_TYPE_HEADER_BAR (celluloid_header_bar_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidHeaderBar, celluloid_header_bar, CELLULOID, HEADER_BAR, GtkBox)

GtkWidget *
celluloid_header_bar_new(void);

gboolean
celluloid_header_bar_get_open_button_popup_visible(CelluloidHeaderBar *hdr);

void
celluloid_header_bar_set_open_button_popup_visible(	CelluloidHeaderBar *hdr,
							gboolean visible );

gboolean
celluloid_header_bar_get_menu_button_popup_visible(CelluloidHeaderBar *hdr);

void
celluloid_header_bar_set_menu_button_popup_visible(	CelluloidHeaderBar *hdr,
							gboolean visible );

void
celluloid_header_bar_set_fullscreen_state(	CelluloidHeaderBar *hdr,
						gboolean fullscreen );

void
celluloid_header_bar_update_track_list(	CelluloidHeaderBar *hdr,
					const GPtrArray *track_list );

void
celluloid_header_bar_update_disc_list(	CelluloidHeaderBar *hdr,
					const GPtrArray *disc_list );

#endif
