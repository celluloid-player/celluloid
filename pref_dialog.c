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

#include "pref_dialog.h"

static void response_handler(GtkDialog *dialog, gint response_id, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(dialog));
}

inline void pref_dialog_set_string(pref_dialog_t *dlg, gchar *buffer)
{
	gtk_entry_set_text(GTK_ENTRY(dlg->opt_entry), buffer);
}

inline const gchar *pref_dialog_get_string(pref_dialog_t *dlg)
{
	return gtk_entry_get_text(GTK_ENTRY(dlg->opt_entry));
}

inline guint64 pref_dialog_get_string_length(pref_dialog_t *dlg)
{
	return gtk_entry_get_text_length(GTK_ENTRY(dlg->opt_entry));
}

void pref_dialog_init(pref_dialog_t *dlg, GtkWindow *parent)
{
	GdkGeometry geom;

	geom.max_width = G_MAXINT;
	geom.max_height = 0;

	dlg->dialog = gtk_dialog_new_with_buttons(	"Preferences",
							parent,
							GTK_DIALOG_MODAL,
							"_Save",
							GTK_RESPONSE_ACCEPT,
							"_Cancel",
							GTK_RESPONSE_REJECT,
							NULL );

	dlg->content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	dlg->content_area
		= gtk_dialog_get_content_area(GTK_DIALOG(dlg->dialog));

	dlg->opt_label = gtk_label_new("MPV Options:");
	dlg->opt_entry = gtk_entry_new();

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg->dialog),
					dlg->dialog,
					&geom,
					GDK_HINT_MAX_SIZE );

	g_signal_connect(	dlg->dialog,
				"response",
				G_CALLBACK(response_handler),
				NULL );

	gtk_container_set_border_width(GTK_CONTAINER(dlg->content_box), 5);

	gtk_container_add(GTK_CONTAINER(dlg->content_area), dlg->content_box);

	gtk_box_pack_start(	GTK_BOX(dlg->content_box),
				dlg->opt_label,
				FALSE,
				FALSE,
				0 );

	gtk_box_pack_start(	GTK_BOX(dlg->content_box),
				dlg->opt_entry,
				TRUE,
				TRUE,
				0 );

	gtk_widget_show_all(dlg->dialog);
}
