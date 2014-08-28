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

#include "pref_dialog.h"

static void response_handler(	GtkDialog *dialog,
				gint response_id,
				gpointer data );

static void pref_dialog_init(PrefDialog *dlg);

static void response_handler(GtkDialog *dialog, gint response_id, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(dialog));
}

static void pref_dialog_init(PrefDialog *dlg)
{
	GdkGeometry geom;

	geom.max_width = G_MAXINT;
	geom.max_height = 0;

	dlg->content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	dlg->content_area
		= gtk_dialog_get_content_area(GTK_DIALOG(dlg));

	dlg->opt_label = gtk_label_new("MPV Options:");
	dlg->opt_entry = gtk_entry_new();

	gtk_dialog_add_buttons(	GTK_DIALOG(dlg),
				"_Save",
				GTK_RESPONSE_ACCEPT,
				"_Cancel",
				GTK_RESPONSE_REJECT,
				NULL );

	gtk_window_set_modal(GTK_WINDOW(dlg), 1);
	gtk_window_set_title(GTK_WINDOW(dlg), "Preferences");
	gtk_container_set_border_width(GTK_CONTAINER(dlg->content_box), 5);

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg),
					GTK_WIDGET(dlg),
					&geom,
					GDK_HINT_MAX_SIZE );

	g_signal_connect(	dlg,
				"response",
				G_CALLBACK(response_handler),
				NULL );

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

	gtk_widget_show_all(GTK_WIDGET(dlg));
}

GtkWidget *pref_dialog_new(GtkWindow *parent)
{
	PrefDialog *dlg = g_object_new(pref_dialog_get_type(), NULL);

	gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);

	return GTK_WIDGET(dlg);
}

GType pref_dialog_get_type()
{
	static GType dlg_type = 0;

	if(dlg_type == 0)
	{
		const GTypeInfo dlg_info
			= {	sizeof(PrefDialogClass),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				sizeof(PrefDialog),
				0,
				(GInstanceInitFunc)pref_dialog_init };

		dlg_type = g_type_register_static(	GTK_TYPE_DIALOG,
							"PrefDialog",
							&dlg_info,
							0 );
	}

	return dlg_type;
}

void pref_dialog_set_string(PrefDialog *dlg, gchar *buffer)
{
	gtk_entry_set_text(GTK_ENTRY(dlg->opt_entry), buffer);
}

const gchar *pref_dialog_get_string(PrefDialog *dlg)
{
	return gtk_entry_get_text(GTK_ENTRY(dlg->opt_entry));
}

guint64 pref_dialog_get_string_length(PrefDialog *dlg)
{
	return gtk_entry_get_text_length(GTK_ENTRY(dlg->opt_entry));
}
