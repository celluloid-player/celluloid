/*
 * Copyright (c) 2025 gnome-mpv
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

#include "celluloid-file-dialog.h"
#include "celluloid-def.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

static void
load_last_folder(CelluloidFileDialog *dialog);

static void
load_last_folder(CelluloidFileDialog *dialog)
{
	GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
	gchar *uri = g_settings_get_string(settings, "last-folder-uri");

	if(uri && *uri)
	{
		GFile *folder = g_file_new_for_uri(uri);

		gtk_file_dialog_set_initial_folder
			(GTK_FILE_DIALOG(dialog), folder);

		g_object_unref(folder);
	}

	g_free(uri);
	g_object_unref(settings);
}

void
celluloid_file_dialog_set_default_filters(	CelluloidFileDialog *self,
						gboolean audio,
						gboolean video,
						gboolean image,
						gboolean subtitle )
{
	GtkFileDialog *dialog = GTK_FILE_DIALOG(self);
	GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);

	if(audio || video || image || subtitle)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, _("All Files"));
		gtk_file_filter_add_pattern(filter, "*");

		g_list_store_append(filters, G_OBJECT(filter));
	}

	if(audio)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, _("Audio Files"));
		gtk_file_filter_add_mime_type(filter, "audio/*");

		g_list_store_append(filters, G_OBJECT(filter));
		gtk_file_dialog_set_default_filter(dialog, filter);
	}

	if(video)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, _("Video Files"));
		gtk_file_filter_add_mime_type(filter, "video/*");

		g_list_store_append(filters, G_OBJECT(filter));
		gtk_file_dialog_set_default_filter(dialog, filter);
	}

	if(image)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, _("Image Files"));
		gtk_file_filter_add_mime_type(filter, "image/*");

		g_list_store_append(filters, G_OBJECT(filter));
		gtk_file_dialog_set_default_filter(dialog, filter);
	}

	if(subtitle)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		const gchar *exts[] = SUBTITLE_EXTS;

		gtk_file_filter_set_name(filter, _("Subtitle Files"));

		for(gint i = 0; exts[i]; i++)
		{
			gchar *pattern = g_strdup_printf("*.%s", exts[i]);

			gtk_file_filter_add_pattern(filter, pattern);
			g_free(pattern);
		}

		g_list_store_append(filters, G_OBJECT(filter));

		if(!(audio || video || image))
		{
			gtk_file_dialog_set_default_filter(dialog, filter);
		}
	}

	if(audio && video && image)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, _("Media Files"));
		gtk_file_filter_add_mime_type(filter, "audio/*");
		gtk_file_filter_add_mime_type(filter, "video/*");
		gtk_file_filter_add_mime_type(filter, "image/*");

		g_list_store_append(filters, G_OBJECT(filter));
		gtk_file_dialog_set_default_filter(dialog, filter);
	}

	gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));

	g_object_unref(filters);
}

CelluloidFileDialog *
celluloid_file_dialog_new(gboolean restore_state)
{
	CelluloidFileDialog *dialog =
		CELLULOID_FILE_DIALOG(gtk_file_dialog_new());
	GSettings *settings =
		g_settings_new(CONFIG_ROOT);
	const gboolean last_folder_enable =
		g_settings_get_boolean(settings, "last-folder-enable");

	if(restore_state && last_folder_enable)
	{
		load_last_folder(dialog);
	}

	gtk_file_dialog_set_modal(GTK_FILE_DIALOG(dialog), TRUE);

	g_object_unref(settings);

	return dialog;
}
