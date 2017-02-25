/*
 * Copyright (c) 2016-2017 gnome-mpv
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
#include <glib/gprintf.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <locale.h>

#include "gmpv_application.h"
#include "gmpv_application_private.h"
#include "gmpv_actionctl.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_common.h"
#include "gmpv_menu.h"
#include "gmpv_def.h"
#include "mpris/gmpv_mpris.h"
#include "media_keys/gmpv_media_keys.h"

static void update_track_id(	GmpvApplication *app,
				const gchar *action_name,
				const gchar *prop );
static void initialize_gui(GmpvApplication *app);
static gboolean shutdown_signal_handler(gpointer data);
static void activate_handler(GApplication *gapp, gpointer data);
static void open_handler(	GApplication *gapp,
				GFile **files,
				gint n_files,
				gchar *hint,
				gpointer data );
static gint options_handler(	GApplication *gapp,
				GVariantDict *options,
				gpointer data );
static gint command_line_handler(	GApplication *gapp,
					GApplicationCommandLine *cli,
					gpointer data );
static void startup_handler(GApplication *gapp, gpointer data);
static void aid_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void vid_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void sid_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void idle_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void ready_handler(GObject *object, GParamSpec *pspec, gpointer data);
static void message_handler(	GmpvController *controller,
				const gchar *message,
				gpointer data );
static void shutdown_handler(GmpvController *controller, gpointer data);
static void gmpv_application_class_init(GmpvApplicationClass *klass);
static void gmpv_application_init(GmpvApplication *app);

G_DEFINE_TYPE(GmpvApplication, gmpv_application, GTK_TYPE_APPLICATION)

static void update_track_id(	GmpvApplication *app,
				const gchar *action_name,
				const gchar *prop )
{
	GActionMap *action_map = G_ACTION_MAP(app);
	GAction *action = g_action_map_lookup_action(action_map, action_name);
	gint64 id = 0;

	g_object_get(app->controller, prop, &id, NULL);

	if(id >= 0 && action)
	{
		GVariant *value = g_variant_new_int64(id);

		g_action_change_state(action, value);
	}
	else if(!action)
	{
		g_warning("Cannot find action: %s", action_name);
	}
}

static void initialize_gui(GmpvApplication *app)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gboolean csd_enable;
	gboolean always_floating;
	gint64 wid;
	gchar *mpvinput;

	csd_enable =		g_settings_get_boolean
				(settings, "csd-enable");
	always_floating =	g_settings_get_boolean
				(settings, "always-use-floating-controls");
	mpvinput =		g_settings_get_string
				(settings, "mpv-input-config-file");

	app->gui = GMPV_MAIN_WINDOW(gmpv_main_window_new(	app,
								app->playlist,
								always_floating ));

	migrate_config(app);

	if(csd_enable)
	{
		GMenu *app_menu = g_menu_new();

		gmpv_menu_build_app_menu(app_menu);
		gtk_application_set_app_menu
			(GTK_APPLICATION(app), G_MENU_MODEL(app_menu));

		gmpv_main_window_enable_csd(app->gui);
	}
	else
	{
		GMenu *full_menu = g_menu_new();

		gmpv_menu_build_full(full_menu, NULL);
		gtk_application_set_app_menu(GTK_APPLICATION(app), NULL);

		gtk_application_set_menubar
			(GTK_APPLICATION(app), G_MENU_MODEL(full_menu));
	}

	app->view = gmpv_view_new(app->gui);
	wid = gmpv_video_area_get_xid(gmpv_main_window_get_video_area(app->gui));
	app->mpv = gmpv_mpv_new(app->playlist, wid);
	app->model = gmpv_model_new(app->mpv);
	app->controller = gmpv_controller_new(app->model, app->view);

	g_unix_signal_add(SIGHUP, shutdown_signal_handler, app);
	g_unix_signal_add(SIGINT, shutdown_signal_handler, app);
	g_unix_signal_add(SIGTERM, shutdown_signal_handler, app);

	g_signal_connect(	app->controller,
				"notify::aid",
				G_CALLBACK(aid_handler),
				app );
	g_signal_connect(	app->controller,
				"notify::vid",
				G_CALLBACK(vid_handler),
				app );
	g_signal_connect(	app->controller,
				"notify::sid",
				G_CALLBACK(sid_handler),
				app );
	g_signal_connect(	app->controller,
				"notify::idle",
				G_CALLBACK(idle_handler),
				app );
	g_signal_connect(	app->controller,
				"notify::ready",
				G_CALLBACK(ready_handler),
				app );
	g_signal_connect(	app->controller,
				"message",
				G_CALLBACK(message_handler),
				app );
	g_signal_connect(	app->controller,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				app );

	gmpv_actionctl_map_actions(app);
	gmpv_mpris_init(app);
	gmpv_media_keys_init(app);

	g_timeout_add(	SEEK_BAR_UPDATE_INTERVAL,
			(GSourceFunc)update_seek_bar,
			app );

	g_free(mpvinput);
}

static gboolean shutdown_signal_handler(gpointer data)
{
	g_info("Shutdown signal received. Shutting down...");
	gmpv_application_quit(data);

	return FALSE;
}

static void activate_handler(GApplication *gapp, gpointer data)
{
	GmpvApplication *app = data;

	if(!app->gui)
	{
		initialize_gui(app);
	}

	gmpv_controller_present(GMPV_APPLICATION(data)->controller);
}

static void open_handler(	GApplication *gapp,
				GFile **files,
				gint n_files,
				gchar *hint,
				gpointer data )
{
	GmpvApplication *app = data;
	gboolean ready = FALSE;

	if(app->controller)
	{
		g_object_get(app->controller, "ready", &ready, NULL);
	}

	app->enqueue = (g_strcmp0(hint, "enqueue") == 0);

	if(n_files > 0)
	{
		if(ready)
		{
			for(gint i = 0; i < n_files; i++)
			{
				gchar *uri = g_file_get_uri(((GFile **)files)[i]);

				gmpv_controller_open
					(app->controller, uri, i != 0);

				g_free(uri);
			}
		}
		else
		{
			app->files = g_malloc(sizeof(GFile *)*(gsize)(n_files+1));

			for(gint i = 0; i < n_files; i++)
			{
				app->files[i] = g_file_get_uri(((GFile **)files)[i]);
			}

			app->files[n_files] = NULL;
		}
	}
}

static gint options_handler(	GApplication *gapp,
				GVariantDict *options,
				gpointer data )
{
	gboolean version = FALSE;

	g_variant_dict_lookup(options, "version", "b", &version);

	if(version)
	{
		g_printf("GNOME MPV " VERSION "\n");
	}
	else
	{
		GmpvApplication *app = GMPV_APPLICATION(gapp);
		GSettings *settings = g_settings_new(CONFIG_ROOT);

		g_variant_dict_lookup(	options,
					"no-existing-session",
					"b",
					&app->no_existing_session );

		app->no_existing_session
			|=	g_settings_get_boolean
				(	settings,
					"multiple-instances-enable" );

		if(app->no_existing_session)
		{
			GApplicationFlags flags = g_application_get_flags(gapp);

			g_info("Single instance negotiation is disabled");
			g_application_set_flags
				(gapp, flags|G_APPLICATION_NON_UNIQUE);
		}

		g_clear_object(&settings);
	}

	return version?0:-1;
}

static gint command_line_handler(	GApplication *gapp,
					GApplicationCommandLine *cli,
					gpointer data )
{
	gint argc = 1;
	gchar **argv = g_application_command_line_get_arguments(cli, &argc);
	GVariantDict *options = g_application_command_line_get_options_dict(cli);
	gboolean enqueue = FALSE;
	const gint n_files = argc-1;
	GFile *files[n_files];

	g_variant_dict_lookup(options, "enqueue", "b", &enqueue);

	for(gint i = 0; i < n_files; i++)
	{
		files[i] =	g_application_command_line_create_file_for_arg
				(cli, argv[i+1]);
	}

	if(n_files > 0)
	{
		g_application_open(gapp, files, n_files, enqueue?"enqueue":"");
	}

	if(n_files == 0 || !GMPV_APPLICATION(gapp)->gui)
	{
		g_application_activate(gapp);
	}

	for(gint i = 0; i < n_files; i++)
	{
		g_object_unref(files[i]);
	}

	gdk_notify_startup_complete();

	g_strfreev(argv);

	return 0;
}

static void startup_handler(GApplication *gapp, gpointer data)
{
	setlocale(LC_NUMERIC, "C");
	g_set_application_name(_("GNOME MPV"));
	gtk_window_set_default_icon_name(ICON_NAME);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	g_info("Starting GNOME MPV " VERSION);
}

static void aid_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	update_track_id(data,  "set-audio-track", "aid");
}

static void vid_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	update_track_id(data,  "set-video-track", "vid");
}

static void sid_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	update_track_id(data,  "set-subtitle-track", "sid");
}

static void idle_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvApplication *app = data;
	gboolean idle = TRUE;

	g_object_get(object, "idle", &idle, NULL);

	if(!idle && app->inhibit_cookie == 0)
	{
		app->inhibit_cookie
			= gtk_application_inhibit
				(	GTK_APPLICATION(app),
					GTK_WINDOW(app->gui),
					GTK_APPLICATION_INHIBIT_IDLE,
					_("Playing") );
	}
	else if(idle && app->inhibit_cookie != 0)
	{
		gtk_application_uninhibit
			(GTK_APPLICATION(app), app->inhibit_cookie);

		app->inhibit_cookie = 0;
	}
}

static void ready_handler(GObject *object, GParamSpec *pspec, gpointer data)
{
	GmpvApplication *app = data;
	gboolean ready = FALSE;

	g_object_get(object, "ready", &ready, NULL);

	if(ready && app->files)
	{
		for(gint i = 0; app->files[i]; i++)
		{
			gmpv_controller_open
				(app->controller, app->files[i], i != 0);
		}

		g_strfreev(app->files);
		app->files = NULL;
	}
}

static void message_handler(	GmpvController *controller,
				const gchar *message,
				gpointer data )
{
	const gsize prefix_length = sizeof(ACTION_PREFIX)-1;

	if(message && strncmp(message, ACTION_PREFIX, prefix_length) == 0)
	{
		/* Strip prefix and activate */
		activate_action_string(data, message+prefix_length+1);
	}
}

static void shutdown_handler(GmpvController *controller, gpointer data)
{
	gmpv_application_quit(data);
}

static void gmpv_application_class_init(GmpvApplicationClass *klass)
{
}

static void gmpv_application_init(GmpvApplication *app)
{
	app->enqueue = FALSE;
	app->no_existing_session = FALSE;
	app->action_queue = g_queue_new();
	app->files = NULL;
	app->inhibit_cookie = 0;
	app->target_playlist_pos = -1;
	app->playlist = gmpv_playlist_new();
	app->gui = NULL;

	g_application_add_main_option
		(	G_APPLICATION(app),
			"version",
			'\0',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_NONE,
			_("Show release version"),
			NULL );
	g_application_add_main_option
		(	G_APPLICATION(app),
			"enqueue",
			'\0',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_NONE,
			_("Enqueue"),
			NULL );
	g_application_add_main_option
		(	G_APPLICATION(app),
			"no-existing-session",
			'\0',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_NONE,
			_("Don't connect to an already-running instance"),
			NULL );

	g_signal_connect
		(app, "handle-local-options", G_CALLBACK(options_handler), app);
	g_signal_connect
		(app, "command-line", G_CALLBACK(command_line_handler), app);
	g_signal_connect
		(app, "startup", G_CALLBACK(startup_handler), app);
	g_signal_connect
		(app, "activate", G_CALLBACK(activate_handler), app);
	g_signal_connect
		(app, "open", G_CALLBACK(open_handler), app);
}

GmpvApplication *gmpv_application_new(gchar *id, GApplicationFlags flags)
{
	return GMPV_APPLICATION(g_object_new(	gmpv_application_get_type(),
						"application-id", id,
						"flags", flags,
						NULL ));
}

GmpvMainWindow *gmpv_application_get_main_window(GmpvApplication *app)
{
	return app->gui;
}

GmpvMpv *gmpv_application_get_mpv(GmpvApplication *app)
{
	return app->mpv;
}

GmpvPlaylist *gmpv_application_get_playlist(GmpvApplication *app)
{
	return app->playlist;
}

void gmpv_application_quit(GmpvApplication *app)
{
	g_application_quit(G_APPLICATION(app));
}

