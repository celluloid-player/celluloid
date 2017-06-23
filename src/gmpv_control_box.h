/*
 * Copyright (c) 2014-2017 gnome-mpv
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

#ifndef CONTROL_BOX_H
#define CONTROL_BOX_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GMPV_TYPE_CONTROL_BOX (gmpv_control_box_get_type())

G_DECLARE_FINAL_TYPE(GmpvControlBox, gmpv_control_box, GMPV, CONTROL_BOX, GtkBox)

GtkWidget *gmpv_control_box_new(void);
void gmpv_control_box_set_enabled(GmpvControlBox *box, gboolean enabled);
void gmpv_control_box_set_seek_bar_pos(GmpvControlBox *box, gdouble pos);
void gmpv_control_box_set_seek_bar_duration(GmpvControlBox *box, gint duration);
void gmpv_control_box_set_volume(GmpvControlBox *box, gdouble volume);
gdouble gmpv_control_box_get_volume(GmpvControlBox *box);
gboolean gmpv_control_box_get_volume_popup_visible(GmpvControlBox *box);
void gmpv_control_box_set_fullscreen_state(	GmpvControlBox *box,
						gboolean fullscreen );
void gmpv_control_box_set_fullscreen_button_visible(	GmpvControlBox *box,
							gboolean value );
void gmpv_control_box_reset(GmpvControlBox *box);

G_END_DECLS

#endif

