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

#include "gmpv_common.h"
#include "gmpv_def.h"
#include "gmpv_mpv.h"
#include "gmpv_main_window.h"
#include "gmpv_video_area.h"
#include "gmpv_control_box.h"
#include "gmpv_playlist_widget.h"

gchar *get_config_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					NULL );
}

gchar *get_scripts_dir_path(void)
{
	return g_build_filename(	g_get_user_config_dir(),
					CONFIG_DIR,
					"scripts",
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

void activate_action_string(GmpvApplication *app, const gchar *str)
{
	GActionMap *map = G_ACTION_MAP(app);
	GAction *action = NULL;
	gchar *name = NULL;
	GVariant *param = NULL;
	gboolean param_match = FALSE;

	g_action_parse_detailed_name(str, &name, &param, NULL);

	if(name)
	{
		const GVariantType *action_ptype;
		const GVariantType *given_ptype;

		action = g_action_map_lookup_action(map, name);
		action_ptype = g_action_get_parameter_type(action);
		given_ptype = param?g_variant_get_type(param):NULL;

		param_match =	(action_ptype == given_ptype) ||
				g_variant_type_is_subtype_of
				(action_ptype, given_ptype);
	}

	if(action && param_match)
	{
		g_debug("Activating action %s", str);
		g_action_activate(action, param);
	}
	else
	{
		g_warning("Failed to activate action \"%s\"", str);
	}
}

void migrate_config(GmpvApplication *app)
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
	GmpvApplication *app = data;
	GmpvMpv *mpv = gmpv_application_get_mpv(app);
	mpv_handle *mpv_ctx = gmpv_mpv_get_mpv_handle(mpv);
	gdouble time_pos = -1;
	gint rc = -1;

	if(gmpv_mpv_get_state(mpv)->loaded)
	{
		rc = gmpv_mpv_get_property(	mpv,
						"time-pos",
						MPV_FORMAT_DOUBLE,
						&time_pos );
	}

	if(rc >= 0)
	{
		GmpvMainWindow *wnd;
		GmpvControlBox *control_box;

		wnd = gmpv_application_get_main_window(app);
		control_box = gmpv_main_window_get_control_box(wnd);

		gmpv_control_box_set_seek_bar_pos(control_box, time_pos);
	}

	return !!mpv_ctx;
}

void seek(GmpvApplication *app, gdouble time)
{
	const gchar *cmd[] = {"seek", NULL, "absolute", NULL};
	GmpvMpv *mpv = gmpv_application_get_mpv(app);

	if(!gmpv_mpv_get_state(mpv)->loaded)
	{
		gmpv_mpv_load(mpv, NULL, FALSE, TRUE);
	}
	else
	{
		gchar *value_str = g_strdup_printf("%.2f", time);

		cmd[1] = value_str;

		gmpv_mpv_command(mpv, cmd);
		update_seek_bar(app);

		g_free(value_str);
	}

}

void show_error_dialog(GmpvApplication *app, const gchar *prefix, const gchar *msg)
{
	GmpvMainWindow *wnd;
	GtkWidget *dialog;
	GtkWidget *msg_area;
	GList *iter;

	wnd = gmpv_application_get_main_window(app);
	dialog =	gtk_message_dialog_new
			(	GTK_WINDOW(wnd),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("Error") );
	msg_area =	gtk_message_dialog_get_message_area
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

void resize_window_to_fit(GmpvApplication *app, gdouble multiplier)
{
	GmpvMpv *mpv = gmpv_application_get_mpv(app);
	gchar *video = gmpv_mpv_get_property_string(mpv, "video");
	gint64 width;
	gint64 height;
	gint mpv_width_rc;
	gint mpv_height_rc;

	mpv_width_rc = gmpv_mpv_get_property(	mpv,
						"dwidth",
						MPV_FORMAT_INT64,
						&width );
	mpv_height_rc = gmpv_mpv_get_property(	mpv,
						"dheight",
						MPV_FORMAT_INT64,
						&height );

	if(video
	&& strncmp(video, "no", 3) != 0
	&& mpv_width_rc >= 0
	&& mpv_height_rc >= 0)
	{
		GmpvMainWindow *wnd;
		gint new_width;
		gint new_height;

		wnd = gmpv_application_get_main_window(app);
		new_width = (gint)(multiplier*(gdouble)width);
		new_height = (gint)(multiplier*(gdouble)height);

		g_debug("Resizing window to %dx%d", new_width, new_height);

		gmpv_main_window_resize_video_area(wnd, new_width, new_height);
	}

	gmpv_mpv_free(video);
}
