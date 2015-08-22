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

#include "config.h"

#include <glib/gi18n.h>

#include "actionctl.h"
#include "def.h"
#include "mpv.h"
#include "playlist.h"
#include "open_loc_dialog.h"
#include "pref_dialog.h"
#include "common.h"

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

static void open_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	gmpv_handle *ctx = (gmpv_handle*)data;
	GtkFileChooser *file_chooser;
	GtkWidget *open_dialog;

	open_dialog
		= gtk_file_chooser_dialog_new(	_("Open File"),
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
		GSList *uri_list = gtk_file_chooser_get_filenames(file_chooser);
		GSList *uri = uri_list;

		ctx->paused = FALSE;

		while(uri)
		{
			mpv_load(ctx, uri->data, (uri != uri_list), TRUE);

			uri = g_slist_next(uri);
		}

		g_slist_free_full(uri_list, g_free);
	}

	gtk_widget_destroy(open_dialog);
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
	PrefDialog *pref_dialog;
	gboolean csd_enable_buffer;
	gboolean dark_theme_enable_buffer;
	gboolean mpvconf_enable_buffer;
	gboolean mpvinput_enable_buffer;
	gchar *mpvconf_buffer;
	gchar *mpvinput_buffer;
	gchar *mpvopt_buffer;

	csd_enable_buffer
		= g_settings_get_boolean(ctx->config, "csd-enable");

	dark_theme_enable_buffer
		= g_settings_get_boolean(ctx->config, "dark-theme-enable");

	mpvconf_enable_buffer
		= g_settings_get_boolean(ctx->config, "mpv-config-enable");

	mpvinput_enable_buffer
		= g_settings_get_boolean(ctx->config, "mpv-input-config-enable");

	mpvconf_buffer
		= g_settings_get_string(ctx->config, "mpv-config-file");

	mpvinput_buffer
		= g_settings_get_string(ctx->config, "mpv-input-config-file");

	mpvopt_buffer
		= g_settings_get_string(ctx->config, "mpv-options");

	pref_dialog = PREF_DIALOG(pref_dialog_new(GTK_WINDOW(ctx->gui)));

	pref_dialog_set_csd_enable(pref_dialog, csd_enable_buffer);
	pref_dialog_set_dark_theme_enable(pref_dialog, dark_theme_enable_buffer);
	pref_dialog_set_mpvconf_enable(pref_dialog, mpvconf_enable_buffer);
	pref_dialog_set_mpvinput_enable(pref_dialog, mpvinput_enable_buffer);

	pref_dialog_set_mpvconf(pref_dialog, mpvconf_buffer);
	pref_dialog_set_mpvinput(pref_dialog, mpvinput_buffer);
	pref_dialog_set_mpvopt(pref_dialog, mpvopt_buffer);

	g_free(mpvconf_buffer);
	g_free(mpvinput_buffer);
	g_free(mpvopt_buffer);

	if(gtk_dialog_run(GTK_DIALOG(pref_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gboolean csd_enable;
		gboolean dark_theme_enable;
		gboolean mpvconf_enable;
		gboolean mpvinput_enable;
		const gchar* mpvconf;
		const gchar* mpvinput;
		const gchar* mpvopt;
		gint64 playlist_pos;
		gdouble time_pos;
		gint playlist_pos_rc;
		gint time_pos_rc;

		dark_theme_enable
			= pref_dialog_get_dark_theme_enable(pref_dialog);

		csd_enable = pref_dialog_get_csd_enable(pref_dialog);
		mpvconf_enable = pref_dialog_get_mpvconf_enable(pref_dialog);
		mpvinput_enable = pref_dialog_get_mpvinput_enable(pref_dialog);
		mpvconf = pref_dialog_get_mpvconf(pref_dialog);
		mpvinput = pref_dialog_get_mpvinput(pref_dialog);
		mpvopt = pref_dialog_get_mpvopt(pref_dialog);

		g_settings_set_boolean
			(ctx->config, "csd-enable", csd_enable);

		g_settings_set_boolean
			(ctx->config, "dark-theme-enable", dark_theme_enable);

		g_settings_set_boolean
			(ctx->config, "mpv-config-enable", mpvconf_enable);

		g_settings_set_string
			(ctx->config, "mpv-config-file", mpvconf);

		g_settings_set_string
			(ctx->config, "mpv-input-config-file", mpvinput);

		g_settings_set_string
			(ctx->config, "mpv-options", mpvopt);

		g_settings_set_boolean(	ctx->config,
					"mpv-input-config-enable",
					mpvinput_enable );

		if(csd_enable_buffer != csd_enable)
		{
			const gchar * msg
				= _(	"Enabling or disabling client-side "
					"decorations requires restarting %s to "
					"take effect." );

			GtkWidget *dialog
				= gtk_message_dialog_new
					(	GTK_WINDOW(ctx->gui),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO,
						GTK_BUTTONS_OK,
						msg,
						g_get_application_name() );

			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		}

		g_object_set(	ctx->gui->settings,
				"gtk-application-prefer-dark-theme",
				dark_theme_enable,
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

		mpv_terminate_destroy(ctx->mpv_ctx);

		ctx->mpv_ctx = mpv_create();

		mpv_init(ctx, ctx->vid_area_wid);

		mpv_set_wakeup_callback(	ctx->mpv_ctx,
						mpv_wakeup_callback,
						ctx );

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
				mpvinput_enable?mpvinput:NULL,
				mpvinput_enable );
	}

	gtk_widget_destroy(GTK_WIDGET(pref_dialog));
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
				PACKAGE_VERSION,
				"comments",
				_("A GTK frontend for MPV"),
				"license-type",
				GTK_LICENSE_GPL_3_0,
				NULL );
}

void actionctl_map_actions(gmpv_handle *ctx)
{
	const GActionEntry entries[]
		= {	{"open", open_handler, NULL, NULL, NULL},
			{"quit", quit_handler, NULL, NULL, NULL},
			{"about", about_handler, NULL, NULL, NULL},
			{"pref", pref_handler, NULL, NULL, NULL},
			{"openloc", open_loc_handler, NULL, NULL, NULL},
			{"playlist_toggle", playlist_toggle_handler, NULL, NULL, NULL},
			{"playlist_save", playlist_save_handler, NULL, NULL, NULL},
			{"loadsub", load_sub_handler, NULL, NULL, NULL},
			{"fullscreen", fullscreen_handler, NULL, NULL, NULL},
			{"normalsize", normal_size_handler, NULL, NULL, NULL},
			{"doublesize", double_size_handler, NULL, NULL, NULL},
			{"halfsize", half_size_handler, NULL, NULL, NULL} };

	g_action_map_add_action_entries(	G_ACTION_MAP(ctx->app),
						entries,
						G_N_ELEMENTS(entries),
						ctx );
}
