/*
 * Copyright (c) 2016-2021 gnome-mpv
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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#include "celluloid-application.h"
#include "celluloid-controller.h"
#include "celluloid-mpv.h"
#include "celluloid-common.h"
#include "celluloid-def.h"

struct _CelluloidApplication
{
	GtkApplication parent;
	GSList *controllers;
	gboolean enqueue;
	gboolean new_window;
	gchar *mpv_options;
	gchar *role;
	guint inhibit_cookie;
};

struct _CelluloidApplicationClass
{
	GtkApplicationClass parent_class;
};

static void
migrate_config(void);

static void
initialize_gui(CelluloidApplication *app);

static void
create_dirs(void);

static gboolean
shutdown_signal_handler(gpointer data);

static void
new_window_handler(GSimpleAction *simple, GVariant *parameter, gpointer data);

static void
activate_handler(GApplication *gapp, gpointer data);

static void
open_handler(	GApplication *gapp,
		GFile **files,
		gint n_files,
		gchar *hint,
		gpointer data );

static gint
options_handler(GApplication *gapp, GVariantDict *options, gpointer data);

static gboolean
local_command_line(GApplication *gapp, gchar ***arguments, gint *exit_status);

static gint
command_line_handler(	GApplication *gapp,
			GApplicationCommandLine *cli,
			gpointer data );

static void
startup_handler(GApplication *gapp, gpointer data);

static void
idle_handler(GObject *object, GParamSpec *pspec, gpointer data);

static void
shutdown_handler(CelluloidController *controller, gpointer data);

static void
celluloid_application_class_init(CelluloidApplicationClass *klass);

static void
celluloid_application_init(CelluloidApplication *app);

G_DEFINE_TYPE(CelluloidApplication, celluloid_application, GTK_TYPE_APPLICATION)

static void
migrate_config()
{
	const gchar *keys[] = {	"dark-theme-enable",
				"csd-enable",
				"always-use-floating-controls",
				"always-autohide-cursor",
				"use-skip-buttons-for-playlist",
				"last-folder-enable",
				"always-open-new-window",
				"mpv-options",
				"mpv-config-file",
				"mpv-config-enable",
				"mpv-input-config-file",
				"mpv-input-config-enable",
				"mpris-enable",
				"media-keys-enable",
				"prefetch-metadata",
				NULL };

	GSettings *old_settings = g_settings_new("io.github.GnomeMpv");
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

static void
initialize_gui(CelluloidApplication *app)
{
	CelluloidController *controller;
	CelluloidView *view;
	CelluloidMainWindow *window;
	GSettings *settings;

	migrate_config();

	controller = celluloid_controller_new(app);
	view = celluloid_controller_get_view(controller);
	window = celluloid_view_get_main_window(view);
	settings = g_settings_new(CONFIG_ROOT);
	app->controllers = g_slist_prepend(app->controllers, controller);

	#ifdef GDK_WINDOWING_X11
	GtkNative *native = gtk_widget_get_native(GTK_WIDGET(window));
	GdkSurface *surface = gtk_native_get_surface(native);
	GdkDisplay *display = gdk_surface_get_display(surface);

	if(app->role && GDK_IS_X11_DISPLAY(display))
	{
		gdk_x11_surface_set_utf8_property(surface, "WM_ROLE", app->role);
	}
	#endif

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
				"use-skip-buttons-for-playlist",
				controller,
				"use-skip-buttons-for-playlist",
				G_SETTINGS_BIND_GET );
	g_settings_bind(	settings,
				"always-use-floating-controls",
				celluloid_view_get_main_window(view),
				"always-use-floating-controls",
				G_SETTINGS_BIND_GET );
	g_settings_bind(	settings,
				"dark-theme-enable",
				gtk_settings_get_default(),
				"gtk-application-prefer-dark-theme",
				G_SETTINGS_BIND_GET );

	g_object_unref(settings);
}

static void
create_dirs()
{
	gchar *config_dir = get_config_dir_path();
	gchar *scripts_dir = get_scripts_dir_path();
	gchar *watch_dir = get_watch_dir_path();

	g_mkdir_with_parents(config_dir, 0755);
	g_mkdir_with_parents(scripts_dir, 0755);
	g_mkdir_with_parents(watch_dir, 0755);

	g_free(config_dir);
	g_free(scripts_dir);
	g_free(watch_dir);
}

static gboolean
shutdown_signal_handler(gpointer data)
{
	g_info("Shutdown signal received. Shutting down...");
	celluloid_application_quit(data);

	return FALSE;
}

static void
new_window_handler(GSimpleAction *simple, GVariant *parameter, gpointer data)
{
	initialize_gui(data);
}

static void
activate_handler(GApplication *gapp, gpointer data)
{
	CelluloidApplication *app = data;

	if(!app->controllers)
	{
		initialize_gui(app);
	}
}

static void
open_handler(	GApplication *gapp,
		GFile **files,
		gint n_files,
		gchar *hint,
		gpointer data )
{
	CelluloidApplication *app = data;
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	/* Only activate new-window if always-open-new-window is set. It is not
	 * necessary to handle --new-window here since options_handler() would
	 * have activated new-window already if it were set.
	 */
	if(g_settings_get_boolean(settings, "always-open-new-window"))
	{
		activate_action_string(G_ACTION_MAP(gapp), "new-window");
	}

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

	g_object_unref(settings);
}

static gint
options_handler(GApplication *gapp, GVariantDict *options, gpointer data)
{
	gboolean version = FALSE;

	g_variant_dict_lookup(options, "version", "b", &version);

	if(version)
	{
		g_printf("Celluloid " VERSION "\n");
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

static gboolean
local_command_line(GApplication *gapp, gchar ***arguments, gint *exit_status)
{
	CelluloidApplication *app = CELLULOID_APPLICATION(gapp);
	GPtrArray *options = g_ptr_array_new_with_free_func(g_free);
	gchar **argv = *arguments;
	gint i = 1;

	g_clear_pointer(&app->mpv_options, g_free);

	while(argv[i])
	{
		// Ignore --mpv-options
		const gboolean excluded =
			g_str_has_prefix(argv[i], "--mpv-options") &&
			argv[i][sizeof("--mpv-options")] != '=';

		if(!excluded && g_str_has_prefix(argv[i], MPV_OPTION_PREFIX))
		{
			const gchar *suffix =
				argv[i] + sizeof(MPV_OPTION_PREFIX) - 1;

			const gchar *value = strchr(suffix, '=');
			gchar *option = NULL;

			g_assert(!value || value > suffix);

			if(value)
			{
				gchar key[value - suffix + 1];

				g_strlcpy(key, suffix, sizeof(key));

				option = g_strdup_printf("--%s='%s'", key, value + 1);
			}
			else
			{
				option = g_strconcat("--", suffix, NULL);
			}

			g_ptr_array_add(options, option);

			// Remove collected option
			for(gint j = i; argv[j]; j++)
			{
				argv[j] = argv[j + 1];
			}
		}
		else
		{
			i++;
		}
	}

	if(options->len > 0)
	{
		g_ptr_array_add(options, NULL);

		app->mpv_options = g_strjoinv(" ", (gchar **)options->pdata);
	}

	g_ptr_array_free(options, TRUE);

	return	G_APPLICATION_CLASS(celluloid_application_parent_class)
		->local_command_line(gapp, arguments, exit_status);
}

static gint
command_line_handler(	GApplication *gapp,
			GApplicationCommandLine *cli,
			gpointer data )
{
	CelluloidApplication *app = data;
	gint argc = 1;
	gchar **argv = g_application_command_line_get_arguments(cli, &argc);
	GVariantDict *options = g_application_command_line_get_options_dict(cli);
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gboolean always_open_new_window = FALSE;
	gchar *mpv_options = NULL;
	const gint n_files = argc-1;
	GFile *files[n_files];

	app->enqueue = FALSE;
	app->new_window = FALSE;
	always_open_new_window =	g_settings_get_boolean
					(settings, "always-open-new-window");

	g_clear_pointer(&app->role, g_free);

	g_variant_dict_lookup(options, "enqueue", "b", &app->enqueue);
	g_variant_dict_lookup(options, "new-window", "b", &app->new_window);
	g_variant_dict_lookup(options, "mpv-options", "s", &mpv_options);
	g_variant_dict_lookup(options, "role", "s", &app->role);

	/* Combine mpv options from --mpv-options and options matching
	 * MPV_OPTION_PREFIX
	 */
	if(mpv_options)
	{
		gchar *old_mpv_options = app->mpv_options;

		g_warning("--mpv-options is deprecated and will be removed in future versions. Use '--mpv-' prefix options instead.");

		app->mpv_options =
			g_strjoin(" ", mpv_options, app->mpv_options, NULL);

		g_free(old_mpv_options);
		g_free(mpv_options);
	}

	for(gint i = 0; i < n_files; i++)
	{
		files[i] =	g_application_command_line_create_file_for_arg
				(cli, argv[i+1]);
	}

	/* Only activate the new-window action or emit the activate signal if
	 * there are no files to be opened. If there are files, open_handler()
	 * will activate the action by itself. This is necessary because when
	 * opening files from file managers, files to be opened may be sent in
	 * the form of DBus message, bypassing this function entirely.
	 */
	if(app->new_window || (n_files == 0 && always_open_new_window))
	{
		activate_action_string(G_ACTION_MAP(gapp), "new-window");
	}
	else if(n_files == 0 && !app->controllers)
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

	g_object_unref(settings);
	g_strfreev(argv);

	return 0;
}

static void
startup_handler(GApplication *gapp, gpointer data)
{
	g_set_application_name(_("Celluloid"));
	gtk_window_set_default_icon_name(ICON_NAME);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	create_dirs();

	g_info("Starting Celluloid " VERSION);
}

static void
idle_handler(GObject *object, GParamSpec *pspec, gpointer data)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	CelluloidApplication *app = data;
	CelluloidController *controller = NULL;
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

	if(	!idle &&
		app->inhibit_cookie == 0 &&
		g_settings_get_boolean(settings, "inhibit-idle") )
	{
		CelluloidView *view =
			celluloid_controller_get_view(controller);
		CelluloidMainWindow *window =
			celluloid_view_get_main_window(view);

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

	g_object_unref(settings);
}

static void
shutdown_handler(CelluloidController *controller, gpointer data)
{
	CelluloidApplication *app = data;

	app->controllers = g_slist_remove(app->controllers, controller);
	g_object_unref(controller);
	g_free(app->mpv_options);
	g_free(app->role);

	if(!app->controllers)
	{
		celluloid_application_quit(data);
	}
}

static void
celluloid_application_class_init(CelluloidApplicationClass *klass)
{
	G_APPLICATION_CLASS(klass)->local_command_line = local_command_line;
}

static void
celluloid_application_init(CelluloidApplication *app)
{
	GSimpleAction *new_window = g_simple_action_new("new-window", NULL);

	app->controllers = NULL;
	app->enqueue = FALSE;
	app->new_window = FALSE;
	app->mpv_options = NULL;
	app->role = NULL;
	app->inhibit_cookie = 0;

	g_set_prgname(APP_ID);
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
			"mpv-options",
			'\0',
			G_OPTION_FLAG_HIDDEN,
			G_OPTION_ARG_STRING,
			_("Options to pass to mpv"),
			_("OPTIONS") );
	g_application_add_main_option
		(	G_APPLICATION(app),
			"role",
			'\0',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_STRING,
			_("Set the window role"),
			NULL );
	g_application_add_main_option
		(	G_APPLICATION(app),
			"no-existing-session",
			'\0',
			G_OPTION_FLAG_HIDDEN,
			G_OPTION_ARG_NONE,
			_("Don't connect to an already-running instance"),
			NULL );

	// This is only added so that mpv prefix options could be document in
	// the --help text.
	g_application_add_main_option
		(	G_APPLICATION(app),
			"mpv-MPVOPTION",
			'\0',
			G_OPTION_FLAG_NONE,
			G_OPTION_ARG_NONE,
			_("Set the mpv option MPVOPTION to VALUE"),
			_("VALUE") );

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

CelluloidApplication *
celluloid_application_new(gchar *id, GApplicationFlags flags)
{
	const GType type = celluloid_application_get_type();

	return CELLULOID_APPLICATION(g_object_new(	type,
							"application-id", id,
							"flags", flags,
							NULL ));
}

void
celluloid_application_quit(CelluloidApplication *app)
{
	g_application_quit(G_APPLICATION(app));
}

const gchar *
celluloid_application_get_mpv_options(CelluloidApplication *app)
{
	return app->mpv_options;
}
