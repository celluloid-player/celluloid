/*
 * Copyright (c) 2017 gnome-mpv
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

#include <string.h>

#include "gmpv_file_chooser.h"
#include "gmpv_def.h"

static void load_last_folder(GtkFileChooser *chooser);
static void save_last_folder(GtkFileChooser *chooser);
static void response_handler(GtkDialog *dialog, gint response_id, gpointer data);

static void load_last_folder(GtkFileChooser *chooser)
{
	GSettings *win_config = g_settings_new(CONFIG_WIN_STATE);
	gchar *uri = g_settings_get_string(win_config, "last-folder-uri");

	if(uri && *uri)
	{
		gtk_file_chooser_set_current_folder_uri(chooser, uri);
	}

	g_free(uri);
	g_object_unref(win_config);
}

static void save_last_folder(GtkFileChooser *chooser)
{
	gchar *uri = gtk_file_chooser_get_current_folder_uri(chooser);
	GSettings *win_config = g_settings_new(CONFIG_WIN_STATE);

	g_settings_set_string(win_config, "last-folder-uri", uri?:"");

	g_free(uri);
	g_object_unref(win_config);
}

static void response_handler(GtkDialog *dialog, gint response_id, gpointer data)
{
	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GSettings *main_config;
		gboolean last_folder_enable;

		main_config = g_settings_new(CONFIG_ROOT);
		last_folder_enable =	g_settings_get_boolean
					(main_config, "last-folder-enable");

		if(last_folder_enable)
		{
			save_last_folder(GTK_FILE_CHOOSER(dialog));
		}

		g_object_unref(main_config);
	}
}

GmpvFileChooser *gmpv_file_chooser_new(	const gchar *title,
					GtkWindow *parent,
					GtkFileChooserAction action )
{
	GmpvFileChooser *chooser;
	GtkFileChooser *gtk_chooser;
	GSettings *main_config;
	gboolean last_folder_enable;

#if GTK_CHECK_VERSION(3, 19, 7)
	chooser = gtk_file_chooser_native_new(title, parent, action, NULL, NULL);
#else
	chooser = gtk_file_chooser_dialog_new(title, parent, action, NULL);
#endif

	gtk_chooser = GTK_FILE_CHOOSER(chooser);
	main_config = g_settings_new(CONFIG_ROOT);
	last_folder_enable =	g_settings_get_boolean
				(main_config, "last-folder-enable");

	if(last_folder_enable)
	{
		load_last_folder(GTK_FILE_CHOOSER(chooser));
	}

	gmpv_file_chooser_set_modal(chooser, TRUE);
	gtk_file_chooser_set_select_multiple(gtk_chooser, TRUE);
	gtk_file_chooser_set_do_overwrite_confirmation(gtk_chooser, TRUE);

	g_signal_connect(chooser, "response", G_CALLBACK(response_handler), NULL);

	g_object_unref(main_config);

	return chooser;
}

void gmpv_file_chooser_set_default_filters(	GmpvFileChooser *chooser,
						gboolean audio,
						gboolean video,
						gboolean image,
						gboolean subtitle )
{
	GtkFileFilter *filter = gtk_file_filter_new();

	if(audio)
	{
		gtk_file_filter_add_mime_type(filter, "audio/*");
	}

	if(video)
	{
		gtk_file_filter_add_mime_type(filter, "video/*");
	}

	if(image)
	{
		gtk_file_filter_add_mime_type(filter, "image/*");
	}

	if(subtitle)
	{
		const gchar *exts[] = PLAYLIST_EXTS;

		for(gint i = 0; exts[i]; i++)
		{
			gchar *pattern = g_strdup_printf("*.%s", exts[i]);

			gtk_file_filter_add_pattern(filter, pattern);
			g_free(pattern);
		}
	}

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser), filter);
}

