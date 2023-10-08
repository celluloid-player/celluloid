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
	GtkWindow parent_instance;

	GtkWidget *content_box;
	GtkWidget *loc_label;
	GtkWidget *loc_entry;
};

struct _CelluloidOpenLocationDialogClass
{
	GtkWindowClass parent_class;
};

G_DEFINE_TYPE(CelluloidOpenLocationDialog, celluloid_open_location_dialog, GTK_TYPE_WINDOW)

static void
constructed(GObject *object);

static void
key_pressed_handler(	GtkEventControllerKey* controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data);

static void
open_handler(GtkEntry *self, gpointer data);

static void
cancel_handler(GtkEntry *self, gpointer data);

static void
constructed(GObject *object)
{
	CelluloidOpenLocationDialog *dlg =
		CELLULOID_OPEN_LOCATION_DIALOG(object);

	gtk_window_set_modal(GTK_WINDOW(dlg), 1);
	gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
	gtk_window_set_child(GTK_WINDOW(dlg), dlg->content_box);

	GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

	gtk_box_append(GTK_BOX(dlg->content_box), input_box);
	gtk_box_append(GTK_BOX(input_box), dlg->loc_label);
	gtk_box_append(GTK_BOX(input_box), dlg->loc_entry);

	GtkWidget *open_button =
		gtk_button_new_with_label(_("Open"));
	GtkWidget *cancel_button =
		gtk_button_new_with_label(_("Cancel"));

	gtk_widget_add_css_class(open_button, "suggested-action");
	gtk_window_set_default_widget(GTK_WINDOW(dlg), open_button);

	GtkWindow *parent =
		gtk_window_get_transient_for
		(GTK_WINDOW(dlg));
	const gboolean use_header_bar =
		celluloid_main_window_get_csd_enabled
		(CELLULOID_MAIN_WINDOW(parent));

	if(use_header_bar)
	{
		GtkWidget* header_bar =
			gtk_header_bar_new();
		GtkSizeGroup *size_group =
			gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

		gtk_size_group_add_widget(size_group, open_button);
		gtk_size_group_add_widget(size_group, cancel_button);

		gtk_header_bar_pack_start
			(GTK_HEADER_BAR(header_bar), cancel_button);
		gtk_header_bar_pack_end
			(GTK_HEADER_BAR(header_bar), open_button);

		gtk_window_set_titlebar(GTK_WINDOW(dlg), header_bar);
		gtk_widget_set_margin_bottom(dlg->content_box, 12);
	}
	else
	{
		GtkWidget *action_box =
			gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

		gtk_box_append(GTK_BOX(dlg->content_box), action_box);

		gtk_widget_set_halign(action_box, GTK_ALIGN_END);
		gtk_box_set_homogeneous(GTK_BOX(action_box), TRUE);
		gtk_box_append(GTK_BOX(action_box), cancel_button);
		gtk_box_append(GTK_BOX(action_box), open_button);
	}

	gtk_widget_set_margin_start(dlg->content_box, 12);
	gtk_widget_set_margin_end(dlg->content_box, 12);
	gtk_widget_set_margin_top(dlg->content_box, 12);
	gtk_widget_set_margin_bottom(dlg->content_box, 12);

	gtk_widget_set_valign(dlg->content_box, GTK_ALIGN_CENTER);
	gtk_widget_set_vexpand(dlg->content_box, TRUE);
	gtk_widget_set_hexpand(dlg->loc_entry, TRUE);

	gtk_window_set_default_size(GTK_WINDOW(dlg), 350, -1);

	GtkEventController *key_controller = gtk_event_controller_key_new();
	gtk_widget_add_controller(GTK_WIDGET(dlg), key_controller);

	g_signal_connect(	key_controller,
				"key-pressed",
				G_CALLBACK(key_pressed_handler),
				dlg );

	g_signal_connect(	dlg->loc_entry,
				"activate",
				G_CALLBACK(open_handler),
				dlg );
	g_signal_connect(	open_button,
				"clicked",
				G_CALLBACK(open_handler),
				dlg );
	g_signal_connect(	cancel_button,
				"clicked",
				G_CALLBACK(cancel_handler),
				dlg );

	G_OBJECT_CLASS(celluloid_open_location_dialog_parent_class)
		->constructed(object);
}

static void
key_pressed_handler(	GtkEventControllerKey* controller,
			guint keyval,
			guint keycode,
			GdkModifierType state,
			gpointer data)

{
	if(keyval == GDK_KEY_Escape)
	{
		gtk_window_close(GTK_WINDOW(data));
	}
}

static void
open_handler(GtkEntry *self, gpointer data)
{
	g_signal_emit_by_name(data, "response", GTK_RESPONSE_ACCEPT);
	gtk_window_close(GTK_WINDOW(data));
}

static void
cancel_handler(GtkEntry *self, gpointer data)
{
	g_signal_emit_by_name(data, "response", GTK_RESPONSE_CANCEL);
	gtk_window_close(GTK_WINDOW(data));
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
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->constructed = constructed;

	g_signal_new(	"response",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE,
			1,
			G_TYPE_INT );
}

static void
celluloid_open_location_dialog_init(CelluloidOpenLocationDialog *dlg)
{
	dlg->content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
	dlg->loc_label = gtk_label_new(_("Location:"));
	dlg->loc_entry = gtk_entry_new();
}

GtkWidget *
celluloid_open_location_dialog_new(GtkWindow *parent, const gchar *title)
{
	GtkWidget *dlg;
	GtkWidget *header_bar;

	dlg = g_object_new(	celluloid_open_location_dialog_get_type(),
				"transient-for", parent,
				"title", title,
				NULL );

	header_bar = gtk_window_get_titlebar(GTK_WINDOW(dlg));

	if(header_bar)
	{
		gtk_header_bar_set_show_title_buttons
			(GTK_HEADER_BAR(header_bar), FALSE);
	}

	load_text_from_clipboard(CELLULOID_OPEN_LOCATION_DIALOG(dlg));
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
