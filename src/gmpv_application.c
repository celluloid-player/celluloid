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
#include "gmpv_application_action.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_common.h"
#include "gmpv_def.h"

static void migrate_config(void);
static void activate_action_string(GmpvApplication *app, const gchar *str);
static void update_track_id(	GmpvApplication *app,
				const gchar *action_name,
				const gchar *prop );
static void initialize_gui(GmpvApplication *app);
static void mpris_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data );
static void media_keys_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data );
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
static void message_handler(	GmpvController *controller,
				const gchar *message,
				gpointer data );
static void shutdown_handler(GmpvController *controller, gpointer data);
static void gmpv_application_class_init(GmpvApplicationClass *klass);
static void gmpv_application_init(GmpvApplication *app);

G_DEFINE_TYPE(GmpvApplication, gmpv_application, GTK_TYPE_APPLICATION)

static void migrate_config()
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

static void activate_action_string(GmpvApplication *app, const gchar *str)
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
				(	given_ptype &&
					g_variant_type_is_subtype_of
					(action_ptype, given_ptype) );
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
	migrate_config();

	app->controller = gmpv_controller_new(app);
	app->view = gmpv_controller_get_view(app->controller);
	app->gui = gmpv_view_get_main_window(app->view);
	app->model = gmpv_controller_get_model(app->controller);

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
				"message",
				G_CALLBACK(message_handler),
				app );
	g_signal_connect(	app->controller,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				app );
	g_signal_connect(	app->settings,
				"changed::mpris-enable",
				G_CALLBACK(mpris_enable_handler),
				app );
	g_signal_connect(	app->settings,
				"changed::media-keys-enable",
				G_CALLBACK(media_keys_enable_handler),
				app );

	g_settings_bind(	app->settings,
				"always-use-floating-controls",
				app->gui,
				"always-use-floating-controls",
				G_SETTINGS_BIND_GET );
	g_settings_bind(	app->settings,
				"dark-theme-enable",
				gtk_settings_get_default(),
				"gtk-application-prefer-dark-theme",
				G_SETTINGS_BIND_GET );

	gmpv_application_action_add_actions(app);

	if(g_settings_get_boolean(app->settings, "mpris-enable"))
	{
		app->mpris = gmpv_mpris_new(app->controller);
	}

	if(g_settings_get_boolean(app->settings, "media-keys-enable"))
	{
		app->media_keys = gmpv_media_keys_new(app->controller);
	}
}

static void mpris_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data )
{
	GmpvApplication *app = data;

	if(!app->mpris && g_settings_get_boolean(settings, key))
	{
		app->mpris = gmpv_mpris_new(app->controller);
	}
	else if(app->mpris)
	{
		g_clear_object(&app->mpris);
	}
}

static void media_keys_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data )
{
	GmpvApplication *app = data;

	if(!app->media_keys && g_settings_get_boolean(settings, key))
	{
		app->media_keys = gmpv_media_keys_new(app->controller);
	}
	else if(app->media_keys)
	{
		g_clear_object(&app->media_keys);
	}
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

	app->enqueue = (g_strcmp0(hint, "enqueue") == 0);

	for(gint i = 0; i < n_files; i++)
	{
		gchar *uri = g_file_get_uri(((GFile **)files)[i]);
		gboolean append = i != 0 || app->enqueue;

		gmpv_controller_open(app->controller, uri, append);

		g_free(uri);
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

		g_variant_dict_lookup(	options,
					"no-existing-session",
					"b",
					&app->no_existing_session );

		app->no_existing_session
			|=	g_settings_get_boolean
				(	app->settings,
					"multiple-instances-enable" );

		if(app->no_existing_session)
		{
			GApplicationFlags flags = g_application_get_flags(gapp);

			g_info("Single instance negotiation is disabled");
			g_application_set_flags
				(gapp, flags|G_APPLICATION_NON_UNIQUE);
		}
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

	if(n_files == 0 || !GMPV_APPLICATION(gapp)->gui)
	{
		g_application_activate(gapp);
	}

	if(n_files > 0)
	{
		g_application_open(gapp, files, n_files, enqueue?"enqueue":"");
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
	GmpvController *controller = GMPV_APPLICATION(data)->controller;
	GAction *action = g_action_map_lookup_action(data, "set-video-size");
	gint vid = 0;

	g_object_get(controller, "vid", &vid, NULL);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(action), vid > 0);

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
	app->inhibit_cookie = 0;
	app->settings = g_settings_new(CONFIG_ROOT);
	app->gui = NULL;
	app->mpris = NULL;
	app->media_keys = NULL;

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

void gmpv_application_quit(GmpvApplication *app)
{
	g_application_quit(G_APPLICATION(app));
}

