/*
 * Copyright (c) 2014-2016 gnome-mpv
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

#include <gio/gsettingsbackend.h>
#include <glib/gi18n.h>
#include <string.h>

#include "common.h"
#include "def.h"
#include "mpv_obj.h"
#include "main_window.h"
#include "control_box.h"
#include "playlist_widget.h"

gchar *get_config_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					NULL );
}

gchar *get_path_from_uri(const gchar *uri)
{
	GFile *file = g_vfs_get_file_for_uri(g_vfs_get_default(), uri);
	gchar *path = g_file_get_path(file);

	if(file)
	{
		g_object_unref(file);
	}

	return path?path:g_strdup(uri);
}

gchar *get_name_from_path(const gchar *path)
{
	const gchar *scheme = g_uri_parse_scheme(path);
	gchar *basename = NULL;

	/* Check whether the given path is likely to be a local path */
	if(!scheme && path)
	{
		basename = g_path_get_basename(path);
	}

	return basename?basename:g_strdup(path);
}

gboolean quit(gpointer data)
{
	const gchar *cmd[] = {"quit", NULL};
	Application *app = data;

	if(app->mpv->mpv_ctx)
	{
		mpv_obj_command(app->mpv, cmd);
		mpv_obj_quit(app->mpv);

		app->mpv->mpv_ctx = NULL;
	}

	if(!app->gui->fullscreen)
	{
		main_window_save_state(app->gui);
	}

	g_application_quit(G_APPLICATION(app));

	return FALSE;
}

void migrate_config(Application *app)
{
	const gchar *keys[] = {	"dark-theme-enable",
				"csd-enable",
				"last-folder-enable",
				"mpv-options",
				"mpv-config-file",
				"mpv-config-enable",
				"mpv-input-config-file",
				"mpv-input-config-enable",
				NULL };

	GSettings *old_settings = g_settings_new("org.gnome-mpv");
	GSettings *new_settings = g_settings_new(CONFIG_ROOT);

	if(!g_settings_get_boolean(new_settings, "settings-migrated"))
	{
		g_settings_set_boolean(new_settings, "settings-migrated", TRUE);

		for(gint i = 0; keys[i]; i++)
		{
			GVariant *buf = g_settings_get_user_value
						(old_settings, keys[i]);

			if(buf)
			{
				g_settings_set_value
					(new_settings, keys[i], buf);

				g_variant_unref(buf);
			}
		}
	}

	g_object_unref(old_settings);
	g_object_unref(new_settings);
}

gboolean update_seek_bar(gpointer data)
{
	Application *app = data;
	gdouble time_pos = -1;
	gint rc = -1;

	if(app->mpv->mpv_ctx)
	{
		rc = mpv_get_property(	app->mpv->mpv_ctx,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&time_pos );
	}

	if(rc >= 0)
	{
		ControlBox *control_box
			= CONTROL_BOX(app->gui->control_box);

		gtk_range_set_value
			(	GTK_RANGE(control_box->seek_bar),
				time_pos );
	}

	return !!app->mpv->mpv_ctx;
}

void seek(Application *app, gdouble time)
{
	const gchar *cmd[] = {"seek", NULL, "absolute", NULL};

	if(!app->mpv->state.loaded)
	{
		mpv_obj_load(app->mpv, NULL, FALSE, TRUE);
	}
	else
	{
		gchar *value_str = g_strdup_printf("%.2f", time);

		cmd[1] = value_str;

		mpv_obj_command(app->mpv, cmd);
		update_seek_bar(app);

		g_free(value_str);
	}

}

void show_error_dialog(Application *app, const gchar *prefix, const gchar *msg)
{
	GtkWidget *dialog;
	GtkWidget *msg_area;
	GList *iter;

	dialog = gtk_message_dialog_new
			(	GTK_WINDOW(app->gui),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("Error") );

	msg_area = gtk_message_dialog_get_message_area
			(GTK_MESSAGE_DIALOG(dialog));

	iter = gtk_container_get_children(GTK_CONTAINER(msg_area));

	while(iter)
	{
		if(GTK_IS_LABEL(iter->data))
		{
			GtkLabel *label = iter->data;

			gtk_label_set_line_wrap_mode
				(label, PANGO_WRAP_WORD_CHAR);
		}

		iter = g_list_next(iter);
	}

	g_list_free(iter);

	if(prefix)
	{
		gchar *prefix_escaped = g_markup_printf_escaped("%s", prefix);
		gchar *msg_escaped = g_markup_printf_escaped("%s", msg);

		gtk_message_dialog_format_secondary_markup
			(	GTK_MESSAGE_DIALOG(dialog),
				"<b>[%s]</b> %s",
				prefix_escaped,
				msg_escaped );

		g_free(prefix_escaped);
		g_free(msg_escaped);
	}
	else
	{
		gtk_message_dialog_format_secondary_text
			(GTK_MESSAGE_DIALOG(dialog), "%s", msg);
	}

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void resize_window_to_fit(Application *app, gdouble multiplier)
{
	gchar *video = mpv_get_property_string(app->mpv->mpv_ctx, "video");
	gint64 width;
	gint64 height;
	gint mpv_width_rc;
	gint mpv_height_rc;

	mpv_width_rc = mpv_get_property(	app->mpv->mpv_ctx,
						"dwidth",
						MPV_FORMAT_INT64,
						&width );

	mpv_height_rc = mpv_get_property(	app->mpv->mpv_ctx,
						"dheight",
						MPV_FORMAT_INT64,
						&height );

	if(video
	&& strncmp(video, "no", 3) != 0
	&& mpv_width_rc >= 0
	&& mpv_height_rc >= 0)
	{
		gint width_margin;
		gint height_margin;
		gint new_width;
		gint new_height;

		width_margin = main_window_get_width_margin(app->gui);
		height_margin = main_window_get_height_margin(app->gui);
		new_width = (gint)(multiplier*(gdouble)width)+width_margin;
		new_height = (gint)(multiplier*(gdouble)height)+height_margin;

		gtk_window_resize(	GTK_WINDOW(app->gui),
					new_width,
					new_height );
	}

	mpv_free(video);
}

void toggle_fullscreen(Application *app)
{
	main_window_toggle_fullscreen(app->gui);

	mpv_obj_set_property(	app->mpv,
				"fullscreen",
				MPV_FORMAT_FLAG,
				&app->gui->fullscreen );
}
