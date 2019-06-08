/*
 * Copyright (c) 2014-2019 gnome-mpv
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

#ifndef CONTROL_BOX_H
#define CONTROL_BOX_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_CONTROL_BOX (celluloid_control_box_get_type())

G_DECLARE_FINAL_TYPE(CelluloidControlBox, celluloid_control_box, CELLULOID, CONTROL_BOX, GtkBox)

GtkWidget *
celluloid_control_box_new(void);

void
celluloid_control_box_set_enabled(CelluloidControlBox *box, gboolean enabled);

void
celluloid_control_box_set_seek_bar_pos(CelluloidControlBox *box, gdouble pos);

void
celluloid_control_box_set_seek_bar_duration(	CelluloidControlBox *box,
						gint duration );

void
celluloid_control_box_set_volume(CelluloidControlBox *box, gdouble volume);

gdouble
celluloid_control_box_get_volume(CelluloidControlBox *box);

gboolean
celluloid_control_box_get_volume_popup_visible(CelluloidControlBox *box);

void
celluloid_control_box_set_fullscreen_state(	CelluloidControlBox *box,
						gboolean fullscreen );

void
celluloid_control_box_reset(CelluloidControlBox *box);

G_END_DECLS

#endif
