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

#ifndef SEEK_BAR_H
#define SEEK_BAR_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GMPV_TYPE_SEEK_BAR (gmpv_seek_bar_get_type())

G_DECLARE_FINAL_TYPE(GmpvSeekBar, gmpv_seek_bar, GMPV, SEEK_BAR, GtkBox)

GtkWidget *gmpv_seek_bar_new(void);
void gmpv_seek_bar_set_length(GmpvSeekBar *bar, gdouble length);
void gmpv_seek_bar_set_pos(GmpvSeekBar *bar, gdouble pos);

G_END_DECLS

#endif
