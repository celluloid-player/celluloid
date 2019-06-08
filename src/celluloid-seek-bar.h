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

#ifndef SEEK_BAR_H
#define SEEK_BAR_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_SEEK_BAR (celluloid_seek_bar_get_type())

G_DECLARE_FINAL_TYPE(CelluloidSeekBar, celluloid_seek_bar, CELLULOID, SEEK_BAR, GtkBox)

GtkWidget *
celluloid_seek_bar_new(void);

void
celluloid_seek_bar_set_duration(CelluloidSeekBar *bar, gdouble duration);

void
celluloid_seek_bar_set_pos(CelluloidSeekBar *bar, gdouble pos);

G_END_DECLS

#endif
