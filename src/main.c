/*
 * Copyright (c) 2014-2015 gnome-mpv
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

#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <epoxy/gl.h>
#include <locale.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <epoxy/glx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#include <epoxy/egl.h>
#endif
#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>
#include <epoxy/wgl.h>
#endif

#include "def.h"
#include "common.h"
#include "mpv.h"
#include "keybind.h"
#include "playlist.h"
#include "playbackctl.h"
#include "actionctl.h"
#include "main_window.h"
#include "control_box.h"
#include "playlist_widget.h"
#include "pref_dialog.h"
#include "open_loc_dialog.h"
#include "mpris/mpris.h"
#include "media_keys/media_keys.h"

static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean load_files(gpointer data);
static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent* event,
				gpointer data );
static void setup_dnd_targets(gmpv_handle *ctx);
static void connect_signals(gmpv_handle *ctx);
static void add_accelerator(	GtkApplication *app,
				const char *accel,
				const char *action );
static void setup_accelerators(gmpv_handle *ctx);
static GMenu *build_app_menu(void);
static void app_startup_handler(GApplication *app, gpointer data);
static void app_activate_handler(GApplication *app, gpointer data);
static void app_open_handler(	GApplication *app,
				gpointer files,
				gint n_files,
				gchar *hint,
				gpointer data );
static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data);
static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static void playlist_append_handler(	GtkMenuItem *menuitem,
					gpointer data );
static gboolean playlist_mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean vid_area_render_handler(	GtkGLArea *area,
						GdkGLContext *context,
						gpointer data );
static void *get_proc_address(void *fn_ctx, const gchar *name);
static gboolean get_use_opengl(void);
static gint64 get_xid(GtkWidget *widget);

static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	gmpv_handle *ctx = data;
	guint signal_id = g_signal_lookup("draw", MAIN_WINDOW_TYPE);

	g_signal_handlers_disconnect_matched(	widget,
						G_SIGNAL_MATCH_ID
						|G_SIGNAL_MATCH_DATA,
						signal_id,
						0,
						0,
						NULL,
						ctx );

	mpv_init(ctx);
	mpv_set_wakeup_callback(ctx->mpv_ctx, mpv_wakeup_callback, ctx);

	if(!ctx->files)
	{
		control_box_set_enabled
			(CONTROL_BOX(ctx->gui->control_box), FALSE);
	}

	return FALSE;
}

static gboolean load_files(gpointer data)
{
	gmpv_handle *ctx = data;

	if(ctx->files)
	{
		gint i = 0;

		ctx->paused = FALSE;

		playlist_widget_clear(PLAYLIST_WIDGET(ctx->gui->playlist));

		for(i = 0; ctx->files[i]; i++)
		{
			gchar *name = get_name_from_path(ctx->files[i]);

			if(ctx->init_load)
			{
				playlist_widget_append
					(	PLAYLIST_WIDGET
						(ctx->gui->playlist),
						name,
						ctx->files[i] );
			}
			else
			{
				mpv_load(	ctx,
						ctx->files[i],
						(i != 0),
						TRUE );
			}

			g_free(name);
		}

		g_strfreev(ctx->files);
	}

	return FALSE;
}

static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	quit(data);

	return FALSE;
}

static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data)
{
	gboolean append = (widget == ((gmpv_handle *)data)->gui->playlist);

	if(sel_data && gtk_selection_data_get_length(sel_data) > 0)
	{
		gmpv_handle *ctx = data;
		gchar **uri_list = gtk_selection_data_get_uris(sel_data);

		ctx->paused = FALSE;

		if(uri_list)
		{
			int i;

			for(i = 0; uri_list[i]; i++)
			{
				mpv_load(	ctx,
						uri_list[i],
						(append || i != 0),
						TRUE );
			}

			g_strfreev(uri_list);
		}
		else
		{
			const guchar *raw_data
				= gtk_selection_data_get_data(sel_data);

			mpv_load(ctx, (const gchar *)raw_data, append, TRUE);
		}
	}
}

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	gmpv_handle *ctx = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar **command;

	const guint mod_mask =	GDK_MODIFIER_MASK
				&~(GDK_SHIFT_MASK
				|GDK_LOCK_MASK
				|GDK_MOD2_MASK
				|GDK_MOD3_MASK
				|GDK_MOD4_MASK
				|GDK_MOD5_MASK);

	/* Ignore insignificant modifiers (eg. numlock) */
	state &= mod_mask;
	command = keybind_get_command(ctx, FALSE, state, keyval);

	/* Try user-specified keys first, then fallback to hard-coded keys */
	if(command)
	{
		mpv_command(ctx->mpv_ctx, (const char **)command);
	}
	else if((state&mod_mask) == 0)
	{
		/* Accept F11 and f for entering/exiting fullscreen mode. ESC is
		 * only used for exiting fullscreen mode. F11 is handled via
		 * accelrator.
		 */
		if((ctx->gui->fullscreen && keyval == GDK_KEY_Escape)
		|| keyval == GDK_KEY_f)
		{
			GAction *action ;

			action = g_action_map_lookup_action
					(G_ACTION_MAP(ctx->app), "fullscreen");

			g_action_activate(action, NULL);
		}
		else if(keyval == GDK_KEY_Delete
		&& main_window_get_playlist_visible(ctx->gui))
		{
			remove_current_playlist_entry(ctx);
		}
	}

	return FALSE;
}

static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	gmpv_handle *ctx = data;
	GdkEventButton *btn_event = (GdkEventButton *)event;
	gchar **command;

	command = keybind_get_command(	ctx,
					TRUE,
					btn_event->type == GDK_2BUTTON_PRESS,
					btn_event->button );

	if(command)
	{
		mpv_command(ctx->mpv_ctx, (const char **)command);
	}

	return TRUE;
}

static gboolean playlist_mouse_press_handler(	GtkWidget *widget,
						GdkEvent *event,
						gpointer data )
{
	GdkEventButton *btn_event = (GdkEventButton *)event;
	GtkWidget *menu;
	GtkWidget *menuitem;

	if(btn_event->type == GDK_BUTTON_PRESS && btn_event->button == 3) {
		menu = gtk_menu_new();
		menuitem = gtk_menu_item_new_with_label("Append...");

		g_signal_connect(menuitem, "activate", G_CALLBACK(playlist_append_handler), data);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		gtk_widget_show_all(menu);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
					(btn_event != NULL) ? btn_event->button : 0, gdk_event_get_time(event));

		return TRUE;
	}

	return FALSE;
}

static void playlist_append_handler(GtkMenuItem *menuitem, gpointer data)
{
	gboolean last_folder_enable;
	gchar *config_file = get_config_file_path();
	gmpv_handle *ctx = (gmpv_handle*)data;
	GtkFileChooser *file_chooser;
	GtkWidget *append_dialog;
	GSettings *config = NULL;

	append_dialog
		= gtk_file_chooser_dialog_new(	_("Append File"),
						GTK_WINDOW(ctx->gui),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						_("_Cancel"),
						GTK_RESPONSE_CANCEL,
						_("_Open"),
						GTK_RESPONSE_ACCEPT,
						NULL );

	file_chooser = GTK_FILE_CHOOSER(append_dialog);

	last_folder_enable
		= g_settings_get_boolean( 	ctx->config,
						"last-folder-enable" );

	if(last_folder_enable)
	{
		gchar *last_folder_uri = NULL;

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

	if(gtk_dialog_run(GTK_DIALOG(append_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GSList *uri_list = gtk_file_chooser_get_filenames(file_chooser);
		GSList *uri = uri_list;

		ctx->paused = FALSE;

		while(uri)
		{
			mpv_load(ctx, uri->data, TRUE, TRUE);

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

	gtk_widget_destroy(append_dialog);
	g_clear_object(&config);
	g_free(config_file);
}

static void app_activate_handler(GApplication *app, gpointer data)
{
	gtk_window_present(GTK_WINDOW(((gmpv_handle *)data)->gui));
}

static void *get_proc_address(void *fn_ctx, const gchar *name)
{
	GdkDisplay *display = gdk_display_get_default();

#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY(display))
		return eglGetProcAddress(name);
#endif
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(display))
		return (void *)(intptr_t)glXGetProcAddressARB((const GLubyte *)name);
#endif
#ifdef GDK_WINDOWING_WIN32
	if (GDK_IS_WIN32_DISPLAY(display))
		return wglGetProcAddress(name);
#endif

	g_assert_not_reached();
}

static gboolean get_use_opengl(void)
{
#ifdef GDK_WINDOWING_X11
	/* TODO: Add option to use opengl on X11 */
	return !GDK_IS_X11_DISPLAY(gdk_display_get_default());
#else
	/* In theory this can work on any backend supporting GtkGLArea */
	return TRUE;
#endif
}

static gint64 get_xid(GtkWidget *widget)
{
#ifdef GDK_WINDOWING_X11
	return (gint64)gdk_x11_window_get_xid(gtk_widget_get_window(widget));
#else
	return -1;
#endif
}

static gboolean vid_area_render_handler(	GtkGLArea *area,
						GdkGLContext *context,
						gpointer data )
{
	gmpv_handle *ctx = data;
	int width;
	int height;
	int fbo;

	if(!ctx->opengl_ready)
	{
		mpv_check_error(mpv_opengl_cb_init_gl(	ctx->opengl_ctx,
							NULL,
							get_proc_address,
							NULL ));

		ctx->opengl_ready = TRUE;
	}

	width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
	height = (-1)*gtk_widget_get_allocated_height(GTK_WIDGET(area));
	fbo = -1;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
	mpv_opengl_cb_draw(ctx->opengl_ctx, fbo, width, height);

	return TRUE;
}

static void app_open_handler(	GApplication *app,
				gpointer files,
				gint n_files,
				gchar *hint,
				gpointer data )
{
	gmpv_handle *ctx = data;
	gint i;

	if(n_files > 0)
	{
		ctx->files = g_malloc(sizeof(GFile *)*(gsize)(n_files+1));

		for(i = 0; i < n_files; i++)
		{
			ctx->files[i] = g_file_get_uri(((GFile **)files)[i]);
		}

		ctx->files[i] = NULL;

		g_idle_add(load_files, ctx);
	}
}

static void setup_dnd_targets(gmpv_handle *ctx)
{
	PlaylistWidget *playlist;
	GtkTargetEntry target_entry[3];

	playlist = PLAYLIST_WIDGET(ctx->gui->playlist);

	target_entry[0].target = "text/uri-list";
	target_entry[0].flags = 0;
	target_entry[0].info = 0;
	target_entry[1].target = "text/plain";
	target_entry[1].flags = 0;
	target_entry[1].info = 1;
	target_entry[2].target = "STRING";
	target_entry[2].flags = 0;
	target_entry[2].info = 1;

	gtk_drag_dest_set(	GTK_WIDGET(ctx->gui->vid_area),
				GTK_DEST_DEFAULT_ALL,
				target_entry,
				3,
				GDK_ACTION_LINK );

	gtk_drag_dest_set(	GTK_WIDGET(playlist),
				GTK_DEST_DEFAULT_ALL,
				target_entry,
				3,
				GDK_ACTION_COPY );

	gtk_drag_dest_add_uri_targets(GTK_WIDGET(ctx->gui->vid_area));
	gtk_drag_dest_add_uri_targets(GTK_WIDGET(playlist));
}

static void connect_signals(gmpv_handle *ctx)
{
	PlaylistWidget *playlist = PLAYLIST_WIDGET(ctx->gui->playlist);

	playbackctl_connect_signals(ctx);

	if(main_window_get_use_opengl(ctx->gui))
	{
		g_signal_connect(	ctx->gui->vid_area,
					"render",
					G_CALLBACK(vid_area_render_handler),
					ctx );
	}

	g_signal_connect(	ctx->gui->vid_area,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				ctx );

	g_signal_connect(	ctx->gui->playlist,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				ctx );

	g_signal_connect(	ctx->gui,
				"draw",
				G_CALLBACK(draw_handler),
				ctx );

	g_signal_connect(	ctx->gui,
				"delete-event",
				G_CALLBACK(delete_handler),
				ctx );

	g_signal_connect(	ctx->gui,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				ctx );

	g_signal_connect(	ctx->gui->vid_area,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
				ctx );

	g_signal_connect(	playlist->tree_view,
				"row-activated",
				G_CALLBACK(playlist_row_handler),
				ctx );

	g_signal_connect(	playlist->tree_view,
				"button-press-event",
				G_CALLBACK(playlist_mouse_press_handler),
				ctx	);

	g_signal_connect(	playlist->list_store,
				"row-inserted",
				G_CALLBACK(playlist_row_inserted_handler),
				ctx );

	g_signal_connect(	playlist->list_store,
				"row-deleted",
				G_CALLBACK(playlist_row_deleted_handler),
				ctx );
}

static inline void add_accelerator(	GtkApplication *app,
					const char *accel,
					const char *action )
{
	const char *const accels[] = {accel, NULL};

	gtk_application_set_accels_for_action(app, action, accels);
}

static void setup_accelerators(gmpv_handle *ctx)
{
	add_accelerator(ctx->app, "<Control>o", "app.open(false)");
	add_accelerator(ctx->app, "<Control>l", "app.openloc");
	add_accelerator(ctx->app, "<Control>S", "app.playlist_save");
	add_accelerator(ctx->app, "<Control>q", "app.quit");
	add_accelerator(ctx->app, "<Control>p", "app.pref");
	add_accelerator(ctx->app, "F9", "app.playlist_toggle");
	add_accelerator(ctx->app, "<Control>1", "app.normalsize");
	add_accelerator(ctx->app, "<Control>2", "app.doublesize");
	add_accelerator(ctx->app, "<Control>3", "app.halfsize");
	add_accelerator(ctx->app, "F11", "app.fullscreen");
}

static GMenu *build_app_menu()
{
	GMenu *menu;
	GMenu *top_section;
	GMenu *bottom_section;
	GMenuItem *pref_menu_item;
	GMenuItem *about_menu_item;
	GMenuItem *quit_menu_item;

	menu = g_menu_new();
	top_section = g_menu_new();
	bottom_section = g_menu_new();
	pref_menu_item = g_menu_item_new(_("_Preferences"), "app.pref");
	about_menu_item = g_menu_item_new(_("_About"), "app.about");
	quit_menu_item = g_menu_item_new(_("_Quit"), "app.quit");

	g_menu_append_section(menu, NULL, G_MENU_MODEL(top_section));
	g_menu_append_section(menu, NULL, G_MENU_MODEL(bottom_section));
	g_menu_append_item(top_section, pref_menu_item);
	g_menu_append_item(bottom_section, about_menu_item);
	g_menu_append_item(bottom_section, quit_menu_item);

	return menu;
}

static void app_startup_handler(GApplication *app, gpointer data)
{
	gmpv_handle *ctx = data;
	const gchar *vid_area_style = ".gmpv-vid-area{background-color: black}";
	GtkCssProvider *style_provider;
	gboolean css_loaded;
	gboolean use_opengl;
	gboolean config_migrated;
	gboolean mpvinput_enable;
	gboolean csd_enable;
	gboolean dark_theme_enable;
	gchar *mpvinput;

	setlocale(LC_NUMERIC, "C");
	g_set_application_name(_("GNOME MPV"));
	gtk_window_set_default_icon_name(ICON_NAME);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	use_opengl = get_use_opengl();

	ctx->mpv_ctx = mpv_create();
	ctx->files = NULL;
	ctx->opengl_ctx = NULL;
	ctx->opengl_ready = FALSE;
	ctx->paused = TRUE;
	ctx->loaded = FALSE;
	ctx->new_file = TRUE;
	ctx->sub_visible = TRUE;
	ctx->init_load = TRUE;
	ctx->playlist_move_dest = -1;
	ctx->log_level_list = NULL;
	ctx->keybind_list = NULL;
	ctx->config = g_settings_new(CONFIG_ROOT);
	ctx->app = GTK_APPLICATION(app);
	ctx->gui = MAIN_WINDOW(main_window_new(ctx->app, use_opengl));
	ctx->fs_control = NULL;
	ctx->playlist_store = PLAYLIST_WIDGET(ctx->gui->playlist)->list_store;

	config_migrated = migrate_config(ctx);
	style_provider = gtk_css_provider_new();

	css_loaded = gtk_css_provider_load_from_data
			(style_provider, vid_area_style, -1, NULL);

	if(!css_loaded)
	{
		g_warning ("Failed to apply background color css");
	}

	gtk_style_context_add_provider_for_screen
		(	gtk_window_get_screen(GTK_WINDOW(ctx->gui)),
			GTK_STYLE_PROVIDER(style_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

	g_object_unref(style_provider);

	csd_enable = g_settings_get_boolean
				(ctx->config, "csd-enable");

	dark_theme_enable = g_settings_get_boolean
				(ctx->config, "dark-theme-enable");

	mpvinput_enable = g_settings_get_boolean
				(ctx->config, "mpv-input-config-enable");

	mpvinput = g_settings_get_string
				(ctx->config, "mpv-input-config-file");

	if(csd_enable)
	{
		gtk_application_set_app_menu
			(ctx->app, G_MENU_MODEL(build_app_menu()));

		main_window_enable_csd(ctx->gui);
	}
	else
	{
		gtk_application_set_app_menu
			(ctx->app, NULL);

		gtk_application_set_menubar
			(ctx->app, G_MENU_MODEL(build_full_menu()));
	}

	gtk_widget_show_all(GTK_WIDGET(ctx->gui));

	if(csd_enable)
	{
		control_box_set_fullscreen_btn_visible
			(CONTROL_BOX(ctx->gui->control_box), FALSE);
	}

	control_box_set_chapter_enabled
		(CONTROL_BOX(ctx->gui->control_box), FALSE);

	if(!main_window_get_use_opengl(ctx->gui))
	{
		ctx->vid_area_wid = get_xid(ctx->gui->vid_area);
	}

	g_assert(	main_window_get_use_opengl(ctx->gui) ||
			ctx->vid_area_wid != -1 );

	main_window_load_state(ctx->gui);
	setup_accelerators(ctx);
	setup_dnd_targets(ctx);
	actionctl_map_actions(ctx);
	connect_signals(ctx);
	load_keybind(ctx, mpvinput_enable?mpvinput:NULL, FALSE);
	mpris_init(ctx);
	media_keys_init(ctx);

	g_object_set(	ctx->gui->settings,
			"gtk-application-prefer-dark-theme",
			dark_theme_enable,
			NULL );

	g_timeout_add(	SEEK_BAR_UPDATE_INTERVAL,
			(GSourceFunc)update_seek_bar,
			ctx );

	if(config_migrated)
	{
		gchar *config_file;
		gchar *backup_config_file;
		GtkWidget *dialog;

		config_file = get_config_file_path();
		backup_config_file = g_strconcat(config_file, ".bak", NULL);

		dialog = gtk_message_dialog_new
				(	GTK_WINDOW(ctx->gui),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					_("Preferences is now stored using "
					"GSettings. Your preferences have been "
					"migrated from the configuration file. "
					"A backup copy of the configuration "
					"file can be found at \"%s\"."),
					backup_config_file );

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_free(config_file);
		g_free(backup_config_file);
	}

	g_free(mpvinput);
}

int main(int argc, char **argv)
{
	GtkApplication *app;
	gmpv_handle *ctx;
	gint status;

	app = gtk_application_new(APP_ID, G_APPLICATION_HANDLES_OPEN);
	ctx = g_malloc(sizeof(gmpv_handle));

	g_signal_connect(	app,
				"startup",
				G_CALLBACK(app_startup_handler),
				ctx );

	g_signal_connect(	app,
				"activate",
				G_CALLBACK(app_activate_handler),
				ctx );

	g_signal_connect(	app,
				"open",
				G_CALLBACK(app_open_handler),
				ctx );

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}
