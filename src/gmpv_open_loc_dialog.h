/*
 * Copyright (c) 2014 gnome-mpv
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

#ifndef OPEN_LOC_DIALOG_H
#define OPEN_LOC_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GMPV_TYPE_OPEN_LOC_DIALOG (gmpv_open_loc_dialog_get_type ())

G_DECLARE_FINAL_TYPE(GmpvOpenLocDialog, gmpv_open_loc_dialog, GMPV, OPEN_LOC_DIALOG, GtkDialog)

GtkWidget *gmpv_open_loc_dialog_new(GtkWindow *parent);
const gchar *gmpv_open_loc_dialog_get_string(GmpvOpenLocDialog *dlg);
guint64 gmpv_open_loc_dialog_get_string_length(GmpvOpenLocDialog *dlg);

G_END_DECLS

#endif
