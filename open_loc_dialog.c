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

#include "open_loc_dialog.h"

static void response_handler(	GtkDialog *dialog,
				gint response_id,
				gpointer data );

static void open_loc_dialog_init(OpenLocDialog *dlg);

static void response_handler(GtkDialog *dialog, gint response_id, gpointer data)
{
	gtk_widget_hide(GTK_WIDGET(dialog));
}

static void open_loc_dialog_init(OpenLocDialog *dlg)
{
	GdkGeometry geom;

	geom.max_width = G_MAXINT;
	geom.max_height = 0;

	dlg->content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

	dlg->content_area
		= gtk_dialog_get_content_area(GTK_DIALOG(dlg));

	dlg->loc_label = gtk_label_new("Location:");
	dlg->loc_entry = gtk_entry_new();

	gtk_dialog_add_buttons(	GTK_DIALOG(dlg),
				"_Open",
				GTK_RESPONSE_ACCEPT,
				"_Cancel",
				GTK_RESPONSE_REJECT,
				NULL );

	gtk_window_set_geometry_hints(	GTK_WINDOW(dlg),
					GTK_WIDGET(dlg),
					&geom,
					GDK_HINT_MAX_SIZE );

	gtk_window_set_modal(GTK_WINDOW(dlg), 1);
	gtk_window_set_title(GTK_WINDOW(dlg), "Open Location");
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
				dlg->loc_label,
				FALSE,
				FALSE,
				0 );

	gtk_box_pack_start(	GTK_BOX(dlg->content_box),
				dlg->loc_entry,
				TRUE,
				TRUE,
				0 );
}

GtkWidget *open_loc_dialog_new(GtkWindow *parent)
{
	OpenLocDialog *dlg = g_object_new(open_loc_dialog_get_type(), NULL);

	gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
	gtk_widget_show_all(GTK_WIDGET(dlg));

	return GTK_WIDGET(dlg);
}

GType open_loc_dialog_get_type()
{
	static GType dlg_type = 0;

	if(dlg_type == 0)
	{
		const GTypeInfo dlg_info
			= {	sizeof(OpenLocDialogClass),
				NULL,
				NULL,
				NULL,
				NULL,
				NULL,
				sizeof(OpenLocDialog),
				0,
				(GInstanceInitFunc)open_loc_dialog_init };

		dlg_type = g_type_register_static(	GTK_TYPE_DIALOG,
							"OpenLocDialog",
							&dlg_info,
							0 );
	}

	return dlg_type;
}

const gchar *open_loc_dialog_get_string(OpenLocDialog *dlg)
{
	return gtk_entry_get_text(GTK_ENTRY(dlg->loc_entry));
}

guint64 open_loc_dialog_get_string_length(OpenLocDialog *dlg)
{
	return gtk_entry_get_text_length(GTK_ENTRY(dlg->loc_entry));
}
