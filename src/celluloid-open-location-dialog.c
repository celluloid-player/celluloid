/*
 * Copyright (c) 2014-2021, 2023 gnome-mpv
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

#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include "celluloid-open-location-dialog.h"
#include "celluloid-main-window.h"

struct _CelluloidOpenLocationDialog
{
	GtkDialog parent_instance;

	GtkWidget *content_area;
	GtkWidget *content_box;
	GtkWidget *loc_label;
	GtkWidget *loc_entry;
};

struct _CelluloidOpenLocationDialogClass
{
	GtkDialogClass parent_class;
};

G_DEFINE_TYPE(CelluloidOpenLocationDialog, celluloid_open_location_dialog, GTK_TYPE_DIALOG)

static void
entry_activate_handler(GtkEntry *self, gpointer data);

static void
entry_activate_handler(GtkEntry *self, gpointer data)
{
	gtk_dialog_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT);
}

static GPtrArray *
get_clipboards(CelluloidOpenLocationDialog *dlg)
{
	GdkClipboard *clipboards[] =
		{	gtk_widget_get_clipboard(GTK_WIDGET(dlg)),
			gtk_widget_get_primary_clipboard(GTK_WIDGET(dlg)),
			NULL };

	GPtrArray *result = g_ptr_array_new();

	for(gint i = 0; clipboards[i]; i++)
	{
		GdkContentFormats *formats =
			gdk_clipboard_get_formats(clipboards[i]);

		if(gdk_content_formats_contain_mime_type(formats, "text/plain"))
		{
			g_ptr_array_add(result, clipboards[i]);
		}
	}

	return result;
}

static void
clipboard_text_received_handler(	GObject *object,
					GAsyncResult *res,
					gpointer data )
{
	GdkClipboard *clipboard = GDK_CLIPBOARD(object);
	CelluloidOpenLocationDialog *dlg = data;
	gchar *text = gdk_clipboard_read_text_finish(clipboard, res, NULL);

	if(	text &&
		*text &&
		celluloid_open_location_dialog_get_string_length(dlg) == 0 &&
		(g_path_is_absolute(text) || strstr(text, "://") != NULL) )
	{
		GtkEntryBuffer *buffer;

		buffer = gtk_entry_get_buffer(GTK_ENTRY(dlg->loc_entry));
		gtk_entry_buffer_set_text(buffer, text, -1);
		gtk_editable_select_region(GTK_EDITABLE(dlg->loc_entry), 0, -1);
	}

	g_free(text);
	g_object_unref(dlg);
}

static void
load_text_from_clipboard(CelluloidOpenLocationDialog *dlg)
{
	GPtrArray *clipboards = get_clipboards(dlg);

	for(guint i = 0; i < clipboards->len; i++)
	{
		GdkClipboard *clipboard = g_ptr_array_index(clipboards, i);

		g_object_ref(dlg);

		gdk_clipboard_read_text_async
			(clipboard, NULL, clipboard_text_received_handler, dlg);
	}

	g_ptr_array_free(clipboards, FALSE);
}

static void
celluloid_open_location_dialog_class_init(CelluloidOpenLocationDialogClass *klass)
{
}

static void
celluloid_open_location_dialog_init(CelluloidOpenLocationDialog *dlg)
{
	gboolean use_header_bar = TRUE;

	dlg->content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	dlg->content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
	dlg->loc_label = gtk_label_new(_("Location:"));
	dlg->loc_entry = gtk_entry_new();

	g_object_get(G_OBJECT(dlg), "use-header-bar", &use_header_bar, NULL);

	gtk_dialog_add_buttons(	GTK_DIALOG(dlg),
				_("_Open"), GTK_RESPONSE_ACCEPT,
				_("_Cancel"), GTK_RESPONSE_CANCEL,
				NULL );

	gtk_window_set_modal(GTK_WINDOW(dlg), 1);

	if(use_header_bar)
	{
		gtk_widget_set_margin_bottom(dlg->content_box, 12);
	}

	gtk_box_append(GTK_BOX(dlg->content_area), dlg->content_box);
	gtk_box_append(GTK_BOX(dlg->content_box), dlg->loc_label);
	gtk_box_append(GTK_BOX(dlg->content_box), dlg->loc_entry);

	gtk_widget_set_margin_start(dlg->content_box, 12);
	gtk_widget_set_margin_end(dlg->content_box, 12);
	gtk_widget_set_margin_top(dlg->content_box, 12);
	gtk_widget_set_margin_bottom(dlg->content_box, 12);

	gtk_widget_set_valign(dlg->content_box, GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(dlg->content_box, TRUE);
	gtk_widget_set_hexpand(dlg->loc_entry, TRUE);

	gtk_window_set_default_size(GTK_WINDOW(dlg), 350, -1);
	gtk_dialog_set_default_response (GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);

	g_signal_connect(	dlg->loc_entry,
				"activate",
				G_CALLBACK(entry_activate_handler),
				dlg );
}

GtkWidget *
celluloid_open_location_dialog_new(GtkWindow *parent, const gchar *title)
{
	GtkWidget *dlg;
	GtkWidget *header_bar;
	gboolean csd_enabled;

	csd_enabled =	celluloid_main_window_get_csd_enabled
			(CELLULOID_MAIN_WINDOW(parent));

	dlg = g_object_new(	celluloid_open_location_dialog_get_type(),
				"use-header-bar", csd_enabled,
				"title", title,
				NULL );

	header_bar = gtk_dialog_get_header_bar(GTK_DIALOG(dlg));

	if(header_bar)
	{
		gtk_header_bar_set_show_title_buttons
			(GTK_HEADER_BAR(header_bar), FALSE);
	}

	load_text_from_clipboard(CELLULOID_OPEN_LOCATION_DIALOG(dlg));

	gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
	gtk_widget_set_visible(dlg, TRUE);

	return dlg;
}

const gchar *
celluloid_open_location_dialog_get_string(CelluloidOpenLocationDialog *dlg)
{
	GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(dlg->loc_entry));

	return gtk_entry_buffer_get_text(buffer);
}

guint64
celluloid_open_location_dialog_get_string_length(CelluloidOpenLocationDialog *dlg)
{
	return gtk_entry_get_text_length(GTK_ENTRY(dlg->loc_entry));
}
