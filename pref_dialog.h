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

#ifndef PREF_DIALOG_H
#define PREF_DIALOG_H

#include <gtk/gtk.h>

typedef struct pref_dialog_t
{
	GtkWidget *dialog;
	GtkWidget *content_area;
	GtkWidget *content_box;
	GtkWidget *opt_label;
	GtkWidget *opt_entry;
}
pref_dialog_t;

inline void pref_dialog_set_string(pref_dialog_t *dlg, gchar *buffer);
inline const gchar *pref_dialog_get_string(pref_dialog_t *dlg);
inline guint64 pref_dialog_get_string_length(pref_dialog_t *dlg);
void pref_dialog_init(pref_dialog_t *dlg, GtkWindow *parent);

#endif
