/*
 * Copyright (c) 2015 gnome-mpv
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

#include <glib/gi18n.h>

#include "actionctl.h"
#include "def.h"
#include "mpv.h"
#include "playlist.h"
#include "open_loc_dialog.h"
#include "pref_dialog.h"
#include "common.h"

#define G_SETTINGS_KEY_PREF_MAP(VAR, PREF) \
const struct \
{ \
	GType type; \
	const gchar *key; \
	gpointer value; \
} \
VAR[] = {	{G_TYPE_BOOLEAN, "csd-enable", \
		&pref->csd_enable}, \
		{G_TYPE_BOOLEAN, "dark-theme-enable", \
		&pref->dark_theme_enable}, \
		{G_TYPE_BOOLEAN, "last-folder-enable", \
		&pref->last_folder_enable}, \
		{G_TYPE_BOOLEAN, "mpv-config-enable", \
		&pref->mpv_config_enable}, \
		{G_TYPE_BOOLEAN, "mpv-input-config-enable", \
		&pref->mpv_input_config_enable}, \
		{G_TYPE_STRING, "mpv-config-file", \
		&pref->mpv_config_file}, \
		{G_TYPE_STRING, "mpv-input-config-file", \
		&pref->mpv_input_config_file}, \
		{G_TYPE_STRING, "mpv-options", \
		&pref->mpv_options}, \
		{G_TYPE_INVALID, NULL, NULL} };

static void open_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void open_loc_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void pref_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void quit_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void load_sub_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void fullscreen_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void normal_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void double_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void half_size_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void about_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static pref_store *get_pref(GSettings *settings);
static void set_pref(GSettings *settings, pref_store *pref);

static void open_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	gmpv_handle *ctx = data;
	GSettings *config = NULL;
	GtkFileChooser *file_chooser;
	GtkWidget *open_dialog;
	gboolean last_folder_enable;
	gboolean append;

	g_variant_get(param, "b", &append);

	open_dialog
		= gtk_file_chooser_dialog_new(	append?
						_("Add File to Playlist"):
						_("Open File"),
						GTK_WINDOW(ctx->gui),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						_("_Cancel"),
						GTK_RESPONSE_CANCEL,
						_("_Open"),
						GTK_RESPONSE_ACCEPT,
						NULL );

	file_chooser = GTK_FILE_CHOOSER(open_dialog);

	last_folder_enable
		= g_settings_get_boolean( 	ctx->config,
						"last-folder-enable" );

	if(last_folder_enable)
	{
		gchar *last_folder_uri;

		config = g_settings_new(CONFIG_WIN_STATE);

		last_folder_uri = g_settings_get_string
					(config, "last-folder-uri");


		if(last_folder_uri && strlen(last_folder_uri) > 0)
		{
			gtk_file_chooser_set_current_folder_uri
				(file_chooser, last_folder_uri);
		}

		g_free(last_folder_uri);
	}

	gtk_file_chooser_set_select_multiple(file_chooser, TRUE);

	if(gtk_dialog_run(GTK_DIALOG(open_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GSList *uri_list = gtk_file_chooser_get_filenames(file_chooser);
		GSList *uri = uri_list;

		ctx->paused = FALSE;

		while(uri)
		{
			mpv_load(	ctx,
					uri->data,
					(append || uri != uri_list),
					TRUE );

			uri = g_slist_next(uri);
		}

		if(last_folder_enable)
		{
			gchar *last_folder_uri
				= gtk_file_chooser_get_current_folder_uri
					(file_chooser);

			g_settings_set_string
				(config, "last-folder-uri", last_folder_uri);

			g_free(last_folder_uri);
		}

		g_slist_free_full(uri_list, g_free);
	}

	gtk_widget_destroy(open_dialog);
	g_clear_object(&config);
}

static void open_loc_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	gmpv_handle *ctx = (gmpv_handle*)data;
	OpenLocDialog *open_loc_dialog;

	open_loc_dialog
		= OPEN_LOC_DIALOG(open_loc_dialog_new(GTK_WINDOW(ctx->gui)));

	if(gtk_dialog_run(GTK_DIALOG(open_loc_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		const gchar *loc_str;

		loc_str = open_loc_dialog_get_string(open_loc_dialog);

		ctx->paused = FALSE;

		mpv_load(ctx, loc_str, FALSE, TRUE);
	}

	gtk_widget_destroy(GTK_WIDGET(open_loc_dialog));
}

static void pref_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	const gchar *quit_cmd[] = {"quit_watch_later", NULL};
	gmpv_handle *ctx = data;
	pref_store *old_pref;
	pref_store *new_pref;
	PrefDialog *pref_dialog;

	old_pref = get_pref(ctx->config);
	pref_dialog = PREF_DIALOG(pref_dialog_new(GTK_WINDOW(ctx->gui)));

	pref_dialog_set_pref(pref_dialog, old_pref);

	if(gtk_dialog_run(GTK_DIALOG(pref_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gint64 playlist_pos;
		gdouble time_pos;
		gint playlist_pos_rc;
		gint time_pos_rc;

		new_pref = pref_dialog_get_pref(pref_dialog);

		set_pref(ctx->config, new_pref);

		if(old_pref->csd_enable != new_pref->csd_enable)
		{
			GtkWidget *dialog
				= gtk_message_dialog_new
					(	GTK_WINDOW(ctx->gui),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO,
						GTK_BUTTONS_OK,
						_("Enabling or disabling "
						"client-side decorations "
						"requires restarting %s to "
						"take effect." ),
						g_get_application_name() );

			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		}

		g_object_set(	ctx->gui->settings,
				"gtk-application-prefer-dark-theme",
				new_pref->dark_theme_enable,
				NULL );

		mpv_check_error(mpv_set_property_string(	ctx->mpv_ctx,
								"pause",
								"yes" ));

		playlist_pos_rc = mpv_get_property(	ctx->mpv_ctx,
							"playlist-pos",
							MPV_FORMAT_INT64,
							&playlist_pos );

		time_pos_rc = mpv_get_property(	ctx->mpv_ctx,
						"time-pos",
						MPV_FORMAT_DOUBLE,
						&time_pos );

		/* Reset ctx->mpv_ctx */
		mpv_check_error(mpv_command(ctx->mpv_ctx, quit_cmd));

		mpv_quit(ctx);

		ctx->mpv_ctx = mpv_create();

		mpv_init(ctx);

		mpv_set_wakeup_callback(	ctx->mpv_ctx,
						mpv_wakeup_callback,
						ctx );

		gtk_widget_queue_draw(GTK_WIDGET(ctx->gui));

		if(ctx->playlist_store)
		{
			gint rc;

			rc = mpv_request_event(	ctx->mpv_ctx,
						MPV_EVENT_FILE_LOADED,
						0 );

			mpv_check_error(rc);

			mpv_load(ctx, NULL, FALSE, TRUE);

			rc = mpv_request_event(	ctx->mpv_ctx,
						MPV_EVENT_FILE_LOADED,
						1 );

			mpv_check_error(rc);

			if(playlist_pos_rc >= 0 && playlist_pos > 0)
			{
				mpv_set_property(	ctx->mpv_ctx,
							"playlist-pos",
							MPV_FORMAT_INT64,
							&playlist_pos );
			}

			if(time_pos_rc >= 0 && time_pos > 0)
			{
				mpv_set_property(	ctx->mpv_ctx,
							"time-pos",
							MPV_FORMAT_DOUBLE,
							&time_pos );
			}

			mpv_set_property(	ctx->mpv_ctx,
						"pause",
						MPV_FORMAT_FLAG,
						&ctx->paused );
		}

		load_keybind(	ctx,
				new_pref->mpv_input_config_enable?
				new_pref->mpv_input_config_file:NULL,
				new_pref->mpv_input_config_enable );

		pref_store_free(new_pref);
	}

	gtk_widget_destroy(GTK_WIDGET(pref_dialog));
	pref_store_free(old_pref);
}

static void quit_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	quit(data);
}

static void load_sub_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	gmpv_handle *ctx = (gmpv_handle*)data;
	GtkFileChooser *file_chooser;
	GtkWidget *open_dialog;

	open_dialog
		= gtk_file_chooser_dialog_new(	_("Load Subtitle"),
						GTK_WINDOW(ctx->gui),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						_("_Cancel"),
						GTK_RESPONSE_CANCEL,
						_("_Open"),
						GTK_RESPONSE_ACCEPT,
						NULL );

	file_chooser = GTK_FILE_CHOOSER(open_dialog);

	gtk_file_chooser_set_select_multiple(file_chooser, TRUE);

	if(gtk_dialog_run(GTK_DIALOG(open_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		const gchar *cmd[] = {"sub_add", NULL, NULL};
		GSList *uri_list = gtk_file_chooser_get_filenames(file_chooser);
		GSList *uri = uri_list;

		while(uri)
		{
			cmd[1] = uri->data;

			mpv_command(ctx->mpv_ctx, cmd);

			uri = g_slist_next(uri);
		}

		g_slist_free_full(uri_list, g_free);
	}

	gtk_widget_destroy(open_dialog);
}

static void fullscreen_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	toggle_fullscreen(data);
}

static void normal_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	resize_window_to_fit((gmpv_handle *)data, 1);
}

static void double_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	resize_window_to_fit((gmpv_handle *)data, 2);
}

static void half_size_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	resize_window_to_fit((gmpv_handle *)data, 0.5);
}

static void about_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	gmpv_handle *ctx = (gmpv_handle*)data;

	gtk_show_about_dialog(	GTK_WINDOW(ctx->gui),
				"logo-icon-name",
				ICON_NAME,
				"version",
				VERSION,
				"comments",
				_("A GTK frontend for MPV"),
				"license-type",
				GTK_LICENSE_GPL_3_0,
				NULL );
}

static pref_store *get_pref(GSettings *settings)
{
	pref_store *pref = pref_store_new();

	G_SETTINGS_KEY_PREF_MAP(map, pref)

	for(gint i = 0; map[i].key; i++)
	{
		GVariant *value = g_settings_get_value(settings, map[i].key);

		if(map[i].type == G_TYPE_BOOLEAN)
		{
			*((gboolean *)map[i].value)
				= g_variant_get_boolean(value);
		}
		else if(map[i].type == G_TYPE_STRING)
		{
			*((gchar **)map[i].value)
				= g_strdup(g_variant_get_string(value, NULL));
		}
		else
		{
			g_assert_not_reached();
		}

		g_variant_unref(value);
	}

	return pref;
}

static void set_pref(GSettings *settings, pref_store *pref)
{
	G_SETTINGS_KEY_PREF_MAP(map, pref)

	for(gint i = 0; map[i].key; i++)
	{
		GVariant *value = NULL;

		if(map[i].type == G_TYPE_BOOLEAN)
		{
			value = g_variant_new_boolean
				(*((gboolean *)map[i].value));
		}
		else if(map[i].type == G_TYPE_STRING)
		{
			value = g_variant_new_string
				(*((gchar **)map[i].value));
		}
		else
		{
			g_assert_not_reached();
		}

		if(!g_settings_set_value(settings, map[i].key, value))
		{
			g_warning("Failed to set GSettings key %s", map[i].key);
		}
	}
}

void actionctl_map_actions(gmpv_handle *ctx)
{
	const GActionEntry entries[]
	= {	{.name = "open",
		.activate = open_handler,
		.parameter_type = "b"},
		{.name = "quit",
		.activate = quit_handler},
		{.name = "about",
		.activate = about_handler},
		{.name = "pref",
		.activate = pref_handler},
		{.name = "openloc",
		.activate = open_loc_handler},
		{.name = "playlist_toggle",
		.activate = playlist_toggle_handler},
		{.name = "playlist_save",
		.activate = playlist_save_handler},
		{.name = "loadsub",
		.activate = load_sub_handler},
		{.name = "fullscreen",
		.activate = fullscreen_handler},
		{.name = "normalsize",
		.activate = normal_size_handler},
		{.name = "doublesize",
		.activate = double_size_handler},
		{.name = "halfsize",
		.activate = half_size_handler} };

	g_action_map_add_action_entries(	G_ACTION_MAP(ctx->app),
						entries,
						G_N_ELEMENTS(entries),
						ctx );
}
