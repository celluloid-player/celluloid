/*
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

typedef struct open_loc_dialog_t
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *content_box;
	GtkWidget *loc_label;
	GtkWidget *loc_entry;
}
open_loc_dialog_t;

inline const gchar *open_loc_dialog_get_string(open_loc_dialog_t *dlg);
inline guint64 open_loc_dialog_get_string_length(open_loc_dialog_t *dlg);
void open_loc_dialog_init(open_loc_dialog_t *dlg, GtkWindow *parent);

#endif
