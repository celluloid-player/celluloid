/*
 * Copyright (c) 2016 gnome-mpv
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
#include <gdk/gdk.h>
#include <locale.h>

#ifdef OPENGL_CB_ENABLED
#include <epoxy/gl.h>
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
#else
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#else
#error "X11 GDK backend is required when opengl-cb is disabled."
#endif
#endif

#include "application.h"
#include "control_box.h"
#include "actionctl.h"
#include "playbackctl.h"
#include "playlist_widget.h"
#include "keybind.h"
#include "menu.h"
#include "mpv_obj.h"
#include "def.h"
#include "mpris/mpris.h"
#include "media_keys/media_keys.h"

static void startup_handler(GApplication *app, gpointer data);
static void activate_handler(GApplication *app, gpointer data);
static void open_handler(	GApplication *app,
				gpointer files,
				gint n_files,
				gchar *hint,
				gpointer data );
static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean load_files(gpointer data);
static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent* event,
				gpointer data );
static void playlist_row_activated_handler(	GtkTreeView *tree_view,
						GtkTreePath *path,
						GtkTreeViewColumn *column,
						gpointer data );
static void playlist_row_deleted_handler(	Playlist *pl,
						gint pos,
						gpointer data );
static void playlist_row_reodered_handler(	Playlist *pl,
						gint src,
						gint dest,
						gpointer data );
static void mpv_event_handler(	MpvObj *mpv,
				mpv_event_id event_id,
				gpointer data );
static void connect_signals(Application *app);
static void add_accelerator(	GtkApplication *app,
				const char *accel,
				const char *action );
static void setup_accelerators(Application *app);
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
static gboolean get_use_opengl(void);
static gint64 get_xid(GtkWidget *widget);

#ifdef OPENGL_CB_ENABLED
static void *get_proc_address(void *fn_ctx, const gchar *name);
static gboolean vid_area_render_handler(	GtkGLArea *area,
						GdkGLContext *context,
						gpointer data );

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

static gboolean vid_area_render_handler(	GtkGLArea *area,
						GdkGLContext *context,
						gpointer data )
{
	Application *app = data;
	int width;
	int height;
	int fbo;

	if(!app->opengl_ready)
	{
		mpv_check_error(mpv_opengl_cb_init_gl(	app->mpv->opengl_ctx,
							NULL,
							get_proc_address,
							NULL ));

		app->opengl_ready = TRUE;
	}

	width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
	height = (-1)*gtk_widget_get_allocated_height(GTK_WIDGET(area));
	fbo = -1;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
	mpv_opengl_cb_draw(app->mpv->opengl_ctx, fbo, width, height);

	while(gtk_events_pending())
	{
		gtk_main_iteration();
	}

	return TRUE;
}
#endif

static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	Application *app = data;
	guint signal_id = g_signal_lookup("draw", MAIN_WINDOW_TYPE);

	g_signal_handlers_disconnect_matched(	widget,
						G_SIGNAL_MATCH_ID
						|G_SIGNAL_MATCH_DATA,
						signal_id,
						0,
						0,
						NULL,
						app );

	mpv_obj_initialize(app);
	mpv_set_wakeup_callback(app->mpv->mpv_ctx, mpv_obj_wakeup_callback, app);

	if(!app->files)
	{
		control_box_set_enabled
			(CONTROL_BOX(app->gui->control_box), FALSE);
	}

	return FALSE;
}

static gboolean load_files(gpointer data)
{
	Application *app = data;

	if(app->files)
	{
		gint i = 0;

		app->paused = FALSE;

		playlist_clear(app->playlist_store);

		for(i = 0; app->files[i]; i++)
		{
			gchar *name = get_name_from_path(app->files[i]);

			if(app->init_load)
			{
				playlist_append(	app->playlist_store,
							name,
							app->files[i] );
			}
			else
			{
				mpv_obj_load(	app,
						app->files[i],
						(i != 0),
						TRUE );
			}

			g_free(name);
		}

		g_strfreev(app->files);
	}

	return FALSE;
}

static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	quit(data);

	return TRUE;
}

static void playlist_row_activated_handler(	GtkTreeView *tree_view,
						GtkTreePath *path,
						GtkTreeViewColumn *column,
						gpointer data )
{
	Application *app = data;
	gint *indices = gtk_tree_path_get_indices(path);

	if(indices)
	{
		gint64 index = indices[0];

		mpv_obj_set_property(	app->mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&index );
	}
}

static void playlist_row_deleted_handler(	Playlist *pl,
						gint pos,
						gpointer data )
{
	Application *app = data;

	if(app->loaded)
	{
		const gchar *cmd[] = {"playlist_remove", NULL, NULL};
		gchar *index_str = g_strdup_printf("%d", pos);

		cmd[1] = index_str;

		mpv_check_error(mpv_obj_command(app->mpv, cmd));

		g_free(index_str);
	}

	if(playlist_empty(app->playlist_store))
	{
		control_box_set_enabled
			(CONTROL_BOX(app->gui->control_box), FALSE);
	}
}

static void playlist_row_reodered_handler(	Playlist *pl,
						gint src,
						gint dest,
						gpointer data )
{
	Application *app = data;
	const gchar *cmd[] = {"playlist_move", NULL, NULL, NULL};
	gchar *src_str = g_strdup_printf("%d", (src > dest)?--src:src);
	gchar *dest_str = g_strdup_printf("%d", dest);

	cmd[1] = src_str;
	cmd[2] = dest_str;

	mpv_obj_command(app->mpv, cmd);

	g_free(src_str);
	g_free(dest_str);
}

static void mpv_event_handler(	MpvObj *mpv,
				mpv_event_id event_id,
				gpointer data )
{
	Application *app = data;

	if(event_id == MPV_EVENT_VIDEO_RECONFIG)
	{
		if(app->new_file)
		{
			resize_window_to_fit(app, mpv->autofit_ratio);
		}
	}
	else if(event_id == MPV_EVENT_IDLE)
	{
		if(!app->init_load && app->loaded)
		{
			main_window_reset(app->gui);
		}
	}
	else if(event_id == MPV_EVENT_SHUTDOWN)
	{
		quit(app);
	}
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
	gboolean append = (widget == ((Application *)data)->gui->playlist);

	if(sel_data && gtk_selection_data_get_length(sel_data) > 0)
	{
		Application *app = data;
		gchar **uri_list = gtk_selection_data_get_uris(sel_data);

		app->paused = FALSE;

		if(uri_list)
		{
			int i;

			for(i = 0; uri_list[i]; i++)
			{
				mpv_obj_load(	app,
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

			mpv_obj_load(app, (const gchar *)raw_data, append, TRUE);
		}
	}
}

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	Application *app = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *command;

	const guint mod_mask =	GDK_MODIFIER_MASK
				&~(GDK_SHIFT_MASK
				|GDK_LOCK_MASK
				|GDK_MOD2_MASK
				|GDK_MOD3_MASK
				|GDK_MOD4_MASK
				|GDK_MOD5_MASK);

	/* Ignore insignificant modifiers (eg. numlock) */
	state &= mod_mask;
	command = keybind_get_command(app, FALSE, state, keyval);

	/* Try user-specified keys first, then fallback to hard-coded keys */
	if(command)
	{
		mpv_obj_command_string(app->mpv, command);
	}
	else if((state&mod_mask) == 0)
	{
		/* Accept F11 and f for entering/exiting fullscreen mode. ESC is
		 * only used for exiting fullscreen mode. F11 is handled via
		 * accelrator.
		 */
		if((app->gui->fullscreen && keyval == GDK_KEY_Escape)
		|| keyval == GDK_KEY_f)
		{
			GAction *action ;

			action = g_action_map_lookup_action
					(G_ACTION_MAP(app), "fullscreen");

			g_action_activate(action, NULL);
		}
		else if(keyval == GDK_KEY_Delete
		&& main_window_get_playlist_visible(app->gui))
		{
			playlist_widget_remove_selected
				(PLAYLIST_WIDGET(app->gui->playlist));
		}
	}

	return FALSE;
}

static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	Application *app = data;
	GdkEventButton *btn_event = (GdkEventButton *)event;
	gchar *command;

	command = keybind_get_command(	app,
					TRUE,
					btn_event->type == GDK_2BUTTON_PRESS,
					btn_event->button );

	if(command)
	{
		mpv_obj_command_string(app->mpv, command);
	}
	else if(btn_event->button == 1 && btn_event->type == GDK_2BUTTON_PRESS)
	{
		GAction *action;

		action = g_action_map_lookup_action
				(G_ACTION_MAP(app), "fullscreen");

		g_action_activate(action, NULL);
	}

	return TRUE;
}

static gboolean get_use_opengl(void)
{
#if defined(OPENGL_CB_ENABLED) && defined(GDK_WINDOWING_X11)
	/* TODO: Add option to use opengl on X11 */
	return	g_getenv("FORCE_OPENGL_CB") ||
		!GDK_IS_X11_DISPLAY(gdk_display_get_default()) ;
#elif defined(OPENGL_CB_ENABLED)
	/* In theory this can work on any backend supporting GtkGLArea */
	return TRUE;
#else
	return FALSE;
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

static void connect_signals(Application *app)
{
	PlaylistWidget *playlist = PLAYLIST_WIDGET(app->gui->playlist);

	playbackctl_connect_signals(app);

#ifdef OPENGL_CB_ENABLED
	if(main_window_get_use_opengl(app->gui))
	{
		g_signal_connect(	app->gui->vid_area,
					"render",
					G_CALLBACK(vid_area_render_handler),
					app );

		g_signal_connect(	app->gui,
					"draw",
					G_CALLBACK(draw_handler),
					app );
	}
	else
	{
		g_signal_connect_after(	app->gui,
					"draw",
					G_CALLBACK(draw_handler),
					app );
	}
#endif

	g_signal_connect(	app->gui->vid_area,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				app );

	g_signal_connect(	app->gui->playlist,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				app );

	g_signal_connect(	app->gui,
				"delete-event",
				G_CALLBACK(delete_handler),
				app );

	g_signal_connect(	app->gui,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				app );

	g_signal_connect(	app->gui->vid_area,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
				app );

	g_signal_connect(	playlist->tree_view,
				"row-activated",
				G_CALLBACK(playlist_row_activated_handler),
				app );

	g_signal_connect(	playlist->store,
				"row-deleted",
				G_CALLBACK(playlist_row_deleted_handler),
				app );

	g_signal_connect(	playlist->store,
				"row-reordered",
				G_CALLBACK(playlist_row_reodered_handler),
				app );

	g_signal_connect(	app->mpv,
				"mpv-event",
				G_CALLBACK(mpv_event_handler),
				app );
}

static inline void add_accelerator(	GtkApplication *app,
					const char *accel,
					const char *action )
{
	const char *const accels[] = {accel, NULL};

	gtk_application_set_accels_for_action(app, action, accels);
}

static void setup_accelerators(Application *app)
{
	GtkApplication *gtk_app = GTK_APPLICATION(app);

	add_accelerator(gtk_app, "<Control>o", "app.open(false)");
	add_accelerator(gtk_app, "<Control>l", "app.openloc");
	add_accelerator(gtk_app, "<Control>S", "app.playlist_save");
	add_accelerator(gtk_app, "<Control>q", "app.quit");
	add_accelerator(gtk_app, "<Control>p", "app.pref");
	add_accelerator(gtk_app, "F9", "app.playlist_toggle");
	add_accelerator(gtk_app, "<Control>1", "app.normalsize");
	add_accelerator(gtk_app, "<Control>2", "app.doublesize");
	add_accelerator(gtk_app, "<Control>3", "app.halfsize");
	add_accelerator(gtk_app, "F11", "app.fullscreen");
}


G_DEFINE_TYPE(Application, application, GTK_TYPE_APPLICATION)

static void startup_handler(GApplication *gapp, gpointer data)
{
	Application *app = data;
	const gchar *vid_area_style = ".gmpv-vid-area{background-color: black}";
	GtkCssProvider *style_provider;
	gboolean css_loaded;
	gboolean use_opengl;
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

	app->files = NULL;
	app->mpv = mpv_obj_new();
	app->opengl_ready = FALSE;
	app->paused = TRUE;
	app->loaded = FALSE;
	app->new_file = TRUE;
	app->sub_visible = TRUE;
	app->init_load = TRUE;
	app->inhibit_cookie = 0;
	app->keybind_list = NULL;
	app->config = g_settings_new(CONFIG_ROOT);
	app->gui = MAIN_WINDOW(main_window_new(app, app->mpv->playlist, use_opengl));
	app->fs_control = NULL;
	app->playlist_store = PLAYLIST_WIDGET(app->gui->playlist)->store;

	migrate_config(app);

	style_provider = gtk_css_provider_new();

	css_loaded = gtk_css_provider_load_from_data
			(style_provider, vid_area_style, -1, NULL);

	if(!css_loaded)
	{
		g_warning ("Failed to apply background color css");
	}

	gtk_style_context_add_provider_for_screen
		(	gtk_window_get_screen(GTK_WINDOW(app->gui)),
			GTK_STYLE_PROVIDER(style_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

	g_object_unref(style_provider);

	csd_enable = g_settings_get_boolean
				(app->config, "csd-enable");

	dark_theme_enable = g_settings_get_boolean
				(app->config, "dark-theme-enable");

	mpvinput_enable = g_settings_get_boolean
				(app->config, "mpv-input-config-enable");

	mpvinput = g_settings_get_string
				(app->config, "mpv-input-config-file");

	if(csd_enable)
	{
		GMenu *app_menu = g_menu_new();

		menu_build_app_menu(app_menu);

		gtk_application_set_app_menu
			(GTK_APPLICATION(app), G_MENU_MODEL(app_menu));

		main_window_enable_csd(app->gui);
	}
	else
	{
		GMenu *full_menu = g_menu_new();

		menu_build_full(full_menu, NULL, NULL, NULL);
		gtk_application_set_app_menu(GTK_APPLICATION(app), NULL);

		gtk_application_set_menubar
			(GTK_APPLICATION(app), G_MENU_MODEL(full_menu));
	}

	gtk_widget_show_all(GTK_WIDGET(app->gui));

	if(csd_enable)
	{
		control_box_set_fullscreen_btn_visible
			(CONTROL_BOX(app->gui->control_box), FALSE);
	}

	control_box_set_chapter_enabled
		(CONTROL_BOX(app->gui->control_box), FALSE);

	if(!main_window_get_use_opengl(app->gui))
	{
		app->vid_area_wid = get_xid(app->gui->vid_area);
	}

	g_assert(	main_window_get_use_opengl(app->gui) ||
			app->vid_area_wid != -1 );

	main_window_load_state(app->gui);
	setup_accelerators(app);
	actionctl_map_actions(app);
	connect_signals(app);
	load_keybind(app, mpvinput_enable?mpvinput:NULL, FALSE);
	mpris_init(app);
	media_keys_init(app);

	g_object_set(	app->gui->settings,
			"gtk-application-prefer-dark-theme",
			dark_theme_enable,
			NULL );

	g_timeout_add(	SEEK_BAR_UPDATE_INTERVAL,
			(GSourceFunc)update_seek_bar,
			app );

	g_free(mpvinput);
}

static void activate_handler(GApplication *gapp, gpointer data)
{
	gtk_window_present(GTK_WINDOW(APPLICATION(data)->gui));
}

static void open_handler(	GApplication *gapp,
				gpointer files,
				gint n_files,
				gchar *hint,
				gpointer data )
{
	Application *app = data;
	gint i;

	if(n_files > 0)
	{
		app->files = g_malloc(sizeof(GFile *)*(gsize)(n_files+1));

		for(i = 0; i < n_files; i++)
		{
			app->files[i] = g_file_get_uri(((GFile **)files)[i]);
		}

		app->files[i] = NULL;

		g_idle_add(load_files, app);
	}
}

static void application_class_init(ApplicationClass *klass)
{
}

static void application_init(Application *app)
{
	g_signal_connect(app, "startup", G_CALLBACK(startup_handler), app);
	g_signal_connect(app, "activate", G_CALLBACK(activate_handler), app);
	g_signal_connect(app, "open", G_CALLBACK(open_handler), app);
}

Application *application_new(gchar *id, GApplicationFlags flags)
{
	return APPLICATION(g_object_new(	application_get_type(),
						"application-id", id,
						"flags", flags,
						NULL));
}
