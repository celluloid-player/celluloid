/*
 * Copyright (c) 2014, 2016-2017 gnome-mpv
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

#ifndef OPEN_LOCATION_DIALOG_H
#define OPEN_LOCATION_DIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GMPV_TYPE_OPEN_LOC_DIALOG (gmpv_open_location_dialog_get_type ())

G_DECLARE_FINAL_TYPE(GmpvOpenLocationDialog, gmpv_open_location_dialog, GMPV, OPEN_LOCATION_DIALOG, GtkDialog)

GtkWidget *gmpv_open_location_dialog_new(GtkWindow *parent, const gchar *title);
const gchar *gmpv_open_location_dialog_get_string(GmpvOpenLocationDialog *dlg);
guint64 gmpv_open_location_dialog_get_string_length(GmpvOpenLocationDialog *dlg);

G_END_DECLS

#endif
