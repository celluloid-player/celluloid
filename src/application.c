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
#include "track.h"
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
static Track *parse_track_list(mpv_node_list *node);
static void update_track_list(Application *app, mpv_node *track_list);
static void mpv_load_gui_update(Application *app);
static void mpv_prop_change_handler(mpv_event_property *prop, gpointer data);
static void mpv_event_handler(mpv_event *event, gpointer data);
static void mpv_error_handler(MpvObj *mpv, const gchar *err, gpointer data);
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

	app->mpv->mpv_event_handler = mpv_event_handler;

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

		app->mpv->state.paused = FALSE;

		playlist_clear(app->playlist_store);

		for(i = 0; app->files[i]; i++)
		{
			gchar *name = get_name_from_path(app->files[i]);

			if(app->mpv->state.init_load)
			{
				playlist_append(	app->playlist_store,
							name,
							app->files[i] );
			}
			else
			{
				mpv_obj_load(	app->mpv,
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

	if(app->mpv->state.loaded)
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

static Track *parse_track_list(mpv_node_list *node)
{
	Track *entry = track_new();

	for(gint i = 0; i < node->num; i++)
	{
		if(g_strcmp0(node->keys[i], "type") == 0)
		{
			const gchar *type = node->values[i].u.string;

			if(g_strcmp0(type, "audio") == 0)
			{
				entry->type = TRACK_TYPE_AUDIO;
			}
			else if(g_strcmp0(type, "video") == 0)
			{
				entry->type = TRACK_TYPE_VIDEO;
			}
			else if(g_strcmp0(type, "sub") == 0)
			{
				entry->type = TRACK_TYPE_SUBTITLE;
			}
		}
		else if(g_strcmp0(node->keys[i], "title") == 0)
		{
			entry->title = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "lang") == 0)
		{
			entry->lang = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "id") == 0)
		{
			entry->id = node->values[i].u.int64;
		}
	}

	return entry;
}

static void update_track_list(Application *app, mpv_node* track_list)
{
	MpvObj *mpv = app->mpv;
	mpv_node_list *org_list = track_list->u.list;
	GSList *audio_list = NULL;
	GSList *video_list = NULL;
	GSList *sub_list = NULL;
	GAction *action = NULL;
	gint64 aid = -1;
	gint64 sid = -1;

	mpv_get_property(	mpv->mpv_ctx,
				"aid",
				MPV_FORMAT_INT64,
				&aid );

	mpv_get_property(	mpv->mpv_ctx,
				"sid",
				MPV_FORMAT_INT64,
				&sid );

	action = g_action_map_lookup_action
			(G_ACTION_MAP(app), "audio_select");

	g_simple_action_set_state
		(G_SIMPLE_ACTION(action), g_variant_new_int64(aid));

	action = g_action_map_lookup_action
			(G_ACTION_MAP(app), "sub_select");

	g_simple_action_set_state
		(G_SIMPLE_ACTION(action), g_variant_new_int64(sid));

	for(gint i = 0; i < org_list->num; i++)
	{
		Track *entry;
		GSList **list;

		entry = parse_track_list
			(org_list->values[i].u.list);

		if(entry->type == TRACK_TYPE_AUDIO)
		{
			list = &audio_list;
		}
		else if(entry->type == TRACK_TYPE_VIDEO)
		{
			list = &video_list;
		}
		else if(entry->type == TRACK_TYPE_SUBTITLE)
		{
			list = &sub_list;
		}
		else
		{
			g_assert(FALSE);
		}

		*list = g_slist_prepend(*list, entry);
	}

	audio_list = g_slist_reverse(audio_list);
	video_list = g_slist_reverse(video_list);
	sub_list = g_slist_reverse(sub_list);

	main_window_update_track_list
		(app->gui, audio_list, video_list, sub_list);

	g_slist_free_full
		(audio_list, (GDestroyNotify)track_free);

	g_slist_free_full
		(video_list, (GDestroyNotify)track_free);

	g_slist_free_full
		(sub_list, (GDestroyNotify)track_free);
}

static void mpv_load_gui_update(Application *app)
{
	ControlBox *control_box;
	MpvObj *mpv;
	gchar* title;
	gint64 chapter_count;
	gint64 playlist_pos;
	gdouble length;
	gdouble volume;

	mpv = app->mpv;
	control_box = CONTROL_BOX(app->gui->control_box);
	title = mpv_get_property_string(mpv->mpv_ctx, "media-title");

	if(title)
	{
		gtk_window_set_title(GTK_WINDOW(app->gui), title);

		mpv_free(title);
	}

	mpv_check_error(mpv_set_property(	mpv->mpv_ctx,
						"pause",
						MPV_FORMAT_FLAG,
						&mpv->state.paused));

	if(mpv_get_property(	mpv->mpv_ctx,
				"playlist-pos",
				MPV_FORMAT_INT64,
				&playlist_pos) >= 0)
	{
		playlist_set_indicator_pos(mpv->playlist, (gint)playlist_pos);
	}

	if(mpv_get_property(	mpv->mpv_ctx,
				"chapters",
				MPV_FORMAT_INT64,
				&chapter_count) >= 0)
	{
		control_box_set_chapter_enabled(	control_box,
							(chapter_count > 1) );
	}

	if(mpv_get_property(	mpv->mpv_ctx,
				"volume",
				MPV_FORMAT_DOUBLE,
				&volume) >= 0)
	{
		control_box_set_volume(control_box, volume/100);
	}

	if(mpv_get_property(	mpv->mpv_ctx,
				"length",
				MPV_FORMAT_DOUBLE,
				&length) >= 0)
	{
		control_box_set_seek_bar_length(control_box, (gint)length);
	}

	control_box_set_playing_state(control_box, !mpv->state.paused);
}

static void mpv_prop_change_handler(mpv_event_property *prop, gpointer data)
{
	Application *app = data;
	MpvObj *mpv = app->mpv;

	if(g_strcmp0(prop->name, "pause") == 0)
	{
		mpv_load_gui_update(app);

		if(!mpv->state.paused)
		{
			app->inhibit_cookie
				= gtk_application_inhibit
					(	GTK_APPLICATION(app),
						GTK_WINDOW(app->gui),
						GTK_APPLICATION_INHIBIT_IDLE,
						_("Playing") );
		}
		else if(app->inhibit_cookie != 0)
		{
			gtk_application_uninhibit
				(GTK_APPLICATION(app), app->inhibit_cookie);
		}
	}
	else if(g_strcmp0(prop->name, "track-list") == 0 && prop->data)
	{
		update_track_list(app, prop->data);
	}
	else if(g_strcmp0(prop->name, "volume") == 0
	&& (mpv->state.init_load || mpv->state.loaded))
	{
		ControlBox *control_box = CONTROL_BOX(app->gui->control_box);
		gdouble volume = prop->data?*((double *)prop->data)/100.0:0;

		g_signal_handlers_block_matched
			(	control_box->volume_button,
				G_SIGNAL_MATCH_DATA,
				0, 0, NULL, NULL,
				app );

		control_box_set_volume(control_box, volume);

		g_signal_handlers_unblock_matched
			(	control_box->volume_button,
				G_SIGNAL_MATCH_DATA,
				0, 0, NULL, NULL,
				app );
	}
	else if(g_strcmp0(prop->name, "aid") == 0)
	{
		ControlBox *control_box = CONTROL_BOX(app->gui->control_box);

		/* prop->data == NULL iff there is no audio track */
		gtk_widget_set_sensitive
			(control_box->volume_button, !!prop->data);
	}
	else if(g_strcmp0(prop->name, "fullscreen") == 0)
	{
		int *data = prop->data;
		int fullscreen = data?*data:-1;

		if(fullscreen != -1 && fullscreen != app->gui->fullscreen)
		{
			main_window_toggle_fullscreen(MAIN_WINDOW(app->gui));
		}
	}
	else if(g_strcmp0(prop->name, "eof-reached") == 0
	&& prop->data && *((int *)prop->data) == 1)
	{
		main_window_reset(app->gui);
	}
}

static void mpv_event_handler(mpv_event *event, gpointer data)
{
	Application *app = data;
	MpvObj *mpv = app->mpv;

	if(event->event_id == MPV_EVENT_VIDEO_RECONFIG)
	{
		if(app->mpv->state.new_file)
		{
			resize_window_to_fit(app, mpv->autofit_ratio);
		}
	}
	else if(event->event_id == MPV_EVENT_PROPERTY_CHANGE)
	{
		mpv_prop_change_handler(event->data, data);
	}
	else if(event->event_id == MPV_EVENT_FILE_LOADED)
	{
		control_box_set_enabled
			(CONTROL_BOX(app->gui->control_box), TRUE);

		mpv_load_gui_update(app);
	}
	else if(event->event_id == MPV_EVENT_PLAYBACK_RESTART)
	{
		mpv_load_gui_update(app);
	}
	else if(event->event_id == MPV_EVENT_IDLE)
	{
		if(!app->mpv->state.init_load && !app->mpv->state.loaded)
		{
			main_window_reset(app->gui);
		}
	}
	else if(event->event_id == MPV_EVENT_SHUTDOWN)
	{
		quit(app);
	}
}

static void mpv_error_handler(MpvObj *mpv, const gchar *err, gpointer data)
{
	Application *app = data;

	main_window_reset(app->gui);
	show_error_dialog(app, NULL, err);
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

		app->mpv->state.paused = FALSE;

		if(uri_list)
		{
			int i;

			for(i = 0; uri_list[i]; i++)
			{
				mpv_obj_load(	app->mpv,
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

			mpv_obj_load(app->mpv, (const gchar *)raw_data, append, TRUE);
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
				"mpv-error",
				G_CALLBACK(mpv_error_handler),
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
	app->opengl_ready = FALSE;
	app->inhibit_cookie = 0;
	app->keybind_list = NULL;
	app->config = g_settings_new(CONFIG_ROOT);
	app->playlist_store = playlist_new();
	app->gui = MAIN_WINDOW(main_window_new(app, app->playlist_store, use_opengl));
	app->fs_control = NULL;

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

	/* Due to a GTK bug, get_xid() must not be called when opengl-cb is
	 * enabled or the GtkGLArea will break.
	 */
	app->mpv = mpv_obj_new(use_opengl, use_opengl?-1:get_xid(app->gui->vid_area), app->playlist_store);
	app->mpv->state.paused = TRUE;
	app->mpv->state.loaded = FALSE;
	app->mpv->state.new_file = TRUE;
	app->mpv->state.init_load = TRUE;

	if(csd_enable)
	{
		control_box_set_fullscreen_btn_visible
			(CONTROL_BOX(app->gui->control_box), FALSE);
	}

	control_box_set_chapter_enabled
		(CONTROL_BOX(app->gui->control_box), FALSE);

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
