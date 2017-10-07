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
#include "gmpv_controller.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_common.h"
#include "gmpv_def.h"

struct _GmpvApplication
{
	GtkApplication parent;
	GSList *controllers;
	gboolean enqueue;
	gboolean new_window;
	guint inhibit_cookie;
};

struct _GmpvApplicationClass
{
	GtkApplicationClass parent_class;
};

static void migrate_config(void);
static void initialize_gui(GmpvApplication *app);
static void create_dirs(void);
static gboolean shutdown_signal_handler(gpointer data);
static void new_window_handler(	GSimpleAction *simple,
				GVariant *parameter,
				gpointer data );
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
static void idle_handler(	GObject *object,
				GParamSpec *pspec,
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

static void initialize_gui(GmpvApplication *app)
{
	GmpvController *controller;
	GmpvView *view;
	GSettings *settings;

	migrate_config();

	controller = gmpv_controller_new(app);
	view = gmpv_controller_get_view(controller);
	settings = g_settings_new(CONFIG_ROOT);
	app->controllers = g_slist_prepend(app->controllers, controller);

	g_unix_signal_add(SIGHUP, shutdown_signal_handler, app);
	g_unix_signal_add(SIGINT, shutdown_signal_handler, app);
	g_unix_signal_add(SIGTERM, shutdown_signal_handler, app);

	g_signal_connect(	controller,
				"notify::idle",
				G_CALLBACK(idle_handler),
				app );
	g_signal_connect(	controller,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				app );

	g_settings_bind(	settings,
				"always-use-floating-controls",
				gmpv_view_get_main_window(view),
				"always-use-floating-controls",
				G_SETTINGS_BIND_GET );
	g_settings_bind(	settings,
				"dark-theme-enable",
				gtk_settings_get_default(),
				"gtk-application-prefer-dark-theme",
				G_SETTINGS_BIND_GET );

	g_object_unref(settings);
}

static void create_dirs()
{
	gchar *config_dir = get_config_dir_path();
	gchar *scripts_dir = get_scripts_dir_path();
	gchar *watch_dir = get_watch_dir_path();

	g_mkdir_with_parents(config_dir, 0600);
	g_mkdir_with_parents(scripts_dir, 0700);
	g_mkdir_with_parents(watch_dir, 0600);

	g_free(config_dir);
	g_free(scripts_dir);
	g_free(watch_dir);
}

static gboolean shutdown_signal_handler(gpointer data)
{
	g_info("Shutdown signal received. Shutting down...");
	gmpv_application_quit(data);

	return FALSE;
}

static void new_window_handler(	GSimpleAction *simple,
				GVariant *parameter,
				gpointer data )
{
	initialize_gui(data);
}

static void activate_handler(GApplication *gapp, gpointer data)
{
	GmpvApplication *app = data;

	if(!app->controllers || app->new_window)
	{
		initialize_gui(app);
	}
}

static void open_handler(	GApplication *gapp,
				GFile **files,
				gint n_files,
				gchar *hint,
				gpointer data )
{
	GmpvApplication *app = data;

	g_application_activate(gapp);

	for(gint i = 0; i < n_files; i++)
	{
		GtkApplication *gtkapp = GTK_APPLICATION(gapp);
		GtkWindow *window = gtk_application_get_active_window(gtkapp);
		GActionMap *map = G_ACTION_MAP(window);
		gchar *uri = g_file_get_uri(((GFile **)files)[i]);
		gboolean append = i != 0 || app->enqueue;
		GVariant *param = g_variant_new("(sb)", uri, append);
		GAction *action = g_action_map_lookup_action(map, "open");

		g_action_activate(action, param);

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
		gboolean no_existing_session = FALSE;

		g_variant_dict_lookup(	options,
					"no-existing-session",
					"b",
					&no_existing_session );

		if(no_existing_session)
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
	GmpvApplication *app = data;
	gint argc = 1;
	gchar **argv = g_application_command_line_get_arguments(cli, &argc);
	GVariantDict *options = g_application_command_line_get_options_dict(cli);
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	const gint n_files = argc-1;
	GFile *files[n_files];

	app->enqueue = FALSE;
	app->new_window = FALSE;

	g_variant_dict_lookup(options, "enqueue", "b", &app->enqueue);
	g_variant_dict_lookup(options, "new-window", "b", &app->new_window);

	app->new_window |=	g_settings_get_boolean
				(settings, "always-open-new-window");

	for(gint i = 0; i < n_files; i++)
	{
		files[i] =	g_application_command_line_create_file_for_arg
				(cli, argv[i+1]);
	}

	if(n_files == 0 || !app->controllers || app->new_window)
	{
		g_application_activate(gapp);
	}

	if(n_files > 0)
	{
		g_application_open
			(gapp, files, n_files, app->enqueue?"enqueue":"");
	}

	for(gint i = 0; i < n_files; i++)
	{
		g_object_unref(files[i]);
	}

	gdk_notify_startup_complete();

	g_object_unref(settings);
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
	create_dirs();

	g_info("Starting GNOME MPV " VERSION);
}

static void idle_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvApplication *app = data;
	GmpvController *controller = NULL;
	gboolean idle = TRUE;

	for(	GSList *iter = app->controllers;
		iter && idle;
		iter = g_slist_next(iter) )
	{
		gboolean current = TRUE;

		g_object_get(iter->data, "idle", &current, NULL);
		idle &= current;
		controller = iter->data;
	}

	if(!idle && app->inhibit_cookie == 0)
	{
		GmpvView *view = gmpv_controller_get_view(controller);
		GmpvMainWindow *window = gmpv_view_get_main_window(view);

		app->inhibit_cookie
			= gtk_application_inhibit
				(	GTK_APPLICATION(app),
					GTK_WINDOW(window),
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

static void shutdown_handler(GmpvController *controller, gpointer data)
{
	GmpvApplication *app = data;

	app->controllers = g_slist_remove(app->controllers, controller);
	g_object_unref(controller);

	if(!app->controllers)
	{
		gmpv_application_quit(data);
	}
}

static void gmpv_application_class_init(GmpvApplicationClass *klass)
{
}

static void gmpv_application_init(GmpvApplication *app)
{
	GSimpleAction *new_window = g_simple_action_new("new-window", NULL);

	app->controllers = NULL;
	app->enqueue = FALSE;
	app->new_window = FALSE;
	app->inhibit_cookie = 0;

	g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(new_window));

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
			"new-window",
			'\0',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_NONE,
			_("Create a new window"),
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
		(new_window, "activate", G_CALLBACK(new_window_handler), app);
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

