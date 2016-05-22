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

#include "gmpv_application.h"
#include "gmpv_control_box.h"
#include "gmpv_actionctl.h"
#include "gmpv_playbackctl.h"
#include "gmpv_playlist_widget.h"
#include "gmpv_track.h"
#include "gmpv_menu.h"
#include "gmpv_mpv_obj.h"
#include "gmpv_video_area.h"
#include "gmpv_def.h"
#include "mpris/gmpv_mpris.h"
#include "media_keys/gmpv_media_keys.h"

struct _GmpvApplication
{
	GtkApplication parent;
	GmpvMpvObj *mpv;
	gchar **files;
	guint inhibit_cookie;
	gint64 target_playlist_pos;
	GSettings *config;
	GmpvMainWindow *gui;
	GtkWidget *fs_control;
	GmpvPlaylist *playlist_store;
};

struct _GmpvApplicationClass
{
	GtkApplicationClass parent_class;
};

static void *get_proc_address(void *fn_ctx, const gchar *name);
static gboolean vid_area_render_handler(	GtkGLArea *area,
						GdkGLContext *context,
						gpointer data );
static void startup_handler(GApplication *gapp, gpointer data);
static void activate_handler(GApplication *gapp, gpointer data);
static void open_handler(	GApplication *gapp,
				gpointer files,
				gint n_files,
				gchar *hint,
				gpointer data );
static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data );
static void playlist_row_activated_handler(	GmpvPlaylistWidget *playlist,
						gint64 pos,
						gpointer data );
static void playlist_row_deleted_handler(	GmpvPlaylistWidget *playlist,
						gint pos,
						gpointer data );
static void playlist_row_reodered_handler(	GmpvPlaylistWidget *playlist,
						gint src,
						gint dest,
						gpointer data );
static GmpvTrack *parse_track_list(mpv_node_list *node);
static void update_track_list(GmpvApplication *app, mpv_node* track_list);
static void mpv_prop_change_handler(mpv_event_property *prop, gpointer data);
static void mpv_event_handler(mpv_event *event, gpointer data);
static void mpv_error_handler(GmpvMpvObj *mpv, const gchar *err, gpointer data);
static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data);
static gchar *get_full_keystr(guint keyval, guint state);
static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean key_release_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean scroll_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data );
static gboolean queue_render(GtkGLArea *area);
static void opengl_cb_update_callback(void *cb_ctx);
static void set_playlist_pos(GmpvApplication *app, gint64 pos);
static void set_inhibit_idle(GmpvApplication *app, gboolean inhibit);
static gboolean load_files(GmpvApplication* app, const gchar **files);
static void connect_signals(GmpvApplication *app);
static inline void add_accelerator(	GtkApplication *app,
					const char *accel,
					const char *action );
static void setup_accelerators(GmpvApplication *app);
static void gmpv_application_class_init(GmpvApplicationClass *klass);
static void gmpv_application_init(GmpvApplication *app);

G_DEFINE_TYPE(GmpvApplication, gmpv_application, GTK_TYPE_APPLICATION)

static void *get_proc_address(void *fn_ctx, const gchar *name)
{
	GdkDisplay *display = gdk_display_get_default();

#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY(display))
		return eglGetProcAddress(name);
#endif
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(display))
		return	(void *)(intptr_t)
			glXGetProcAddressARB((const GLubyte *)name);
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
	GmpvApplication *app = data;
	mpv_opengl_cb_context *opengl_ctx;

	opengl_ctx = gmpv_mpv_obj_get_opengl_cb_context(app->mpv);

	if(opengl_ctx)
	{
		int width;
		int height;
		int fbo;

		width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
		height = (-1)*gtk_widget_get_allocated_height(GTK_WIDGET(area));
		fbo = -1;

		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
		mpv_opengl_cb_draw(opengl_ctx, fbo, width, height);
	}

	while(gtk_events_pending())
	{
		gtk_main_iteration();
	}

	return TRUE;
}

static void startup_handler(GApplication *gapp, gpointer data)
{
	GmpvApplication *app = data;
	GmpvControlBox *control_box;
	const gchar *vid_area_style = ".gmpv-vid-area{background-color: black}";
	GtkCssProvider *style_provider;
	gboolean css_loaded;
	gboolean csd_enable;
	gboolean dark_theme_enable;
	gint64 wid;
	gchar *mpvinput;

	setlocale(LC_NUMERIC, "C");
	g_set_application_name(_("GNOME MPV"));
	gtk_window_set_default_icon_name(ICON_NAME);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	app->files = NULL;
	app->inhibit_cookie = 0;
	app->target_playlist_pos = -1;
	app->config = g_settings_new(CONFIG_ROOT);
	app->playlist_store = gmpv_playlist_new();
	app->gui = GMPV_MAIN_WINDOW(gmpv_main_window_new(app, app->playlist_store));
	app->fs_control = NULL;

	migrate_config(app);

	control_box = gmpv_main_window_get_control_box(app->gui);
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
	mpvinput = g_settings_get_string
				(app->config, "mpv-input-config-file");

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

		gmpv_menu_build_full(full_menu, NULL, NULL, NULL);
		gtk_application_set_app_menu(GTK_APPLICATION(app), NULL);

		gtk_application_set_menubar
			(GTK_APPLICATION(app), G_MENU_MODEL(full_menu));
	}

	setup_accelerators(app);
	gmpv_actionctl_map_actions(app);
	gmpv_main_window_load_state(app->gui);
	gtk_widget_show_all(GTK_WIDGET(app->gui));

	if(csd_enable)
	{
		gmpv_control_box_set_fullscreen_btn_visible
			(control_box, FALSE);
	}

	gmpv_control_box_set_chapter_enabled(control_box, FALSE);

	wid = gmpv_video_area_get_xid(gmpv_main_window_get_video_area(app->gui));
	app->mpv = gmpv_mpv_obj_new(app->playlist_store, wid);

	if(csd_enable)
	{
		gmpv_control_box_set_fullscreen_btn_visible
			(control_box, FALSE);
	}

	gmpv_control_box_set_chapter_enabled(control_box, FALSE);

	connect_signals(app);
	gmpv_mpris_init(app);
	gmpv_media_keys_init(app);

	g_object_set(	gtk_settings_get_default(),
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
	gtk_window_present(GTK_WINDOW(GMPV_APPLICATION(data)->gui));
}

static void mpv_init_handler(GmpvMpvObj *mpv, gpointer data)
{
	GmpvApplication *app = data;
	gchar *current_vo = gmpv_mpv_obj_get_property_string(mpv, "current-vo");
	GmpvVideoArea *vid_area = gmpv_main_window_get_video_area(app->gui);

	gmpv_video_area_set_use_opengl(vid_area, !current_vo);

	/* current_vo should be NULL if the selected vo is opengl-cb */
	if(!current_vo)
	{
		GtkGLArea *gl_area;
		mpv_opengl_cb_context *opengl_ctx;
		gint rc;

		gl_area = gmpv_video_area_get_gl_area(vid_area);
		g_signal_connect(	gl_area,
					"render",
					G_CALLBACK(vid_area_render_handler),
					app );

		gtk_gl_area_make_current(gl_area);
		opengl_ctx = gmpv_mpv_obj_get_opengl_cb_context(mpv);
		rc = mpv_opengl_cb_init_gl(	opengl_ctx,
						NULL,
						get_proc_address,
						NULL );

		if(rc >= 0)
		{
			g_debug("Initialized opengl-cb");
		}
		else
		{
			g_critical("Failed to initialize opengl-cb");
		}
	}

	mpv_free(current_vo);
	load_files(app, (const gchar **)app->files);
}

static void open_handler(	GApplication *gapp,
				gpointer files,
				gint n_files,
				gchar *hint,
				gpointer data )
{
	GmpvApplication *app = data;
	gint i;

	if(n_files > 0)
	{
		GmpvMpvObjState state;

		app->files = g_malloc(sizeof(GFile *)*(gsize)(n_files+1));

		for(i = 0; i < n_files; i++)
		{
			app->files[i] = g_file_get_uri(((GFile **)files)[i]);
		}

		app->files[i] = NULL;

		gmpv_mpv_obj_get_state(app->mpv, &state);

		if(state.ready)
		{
			load_files(app, (const gchar **)app->files);
		}
	}

	gdk_notify_startup_complete();
}

static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	GmpvApplication *app = data;
	guint signal_id = g_signal_lookup("draw", GMPV_TYPE_MAIN_WINDOW);

	g_signal_handlers_disconnect_matched(	widget,
						G_SIGNAL_MATCH_ID
						|G_SIGNAL_MATCH_DATA,
						signal_id,
						0,
						0,
						NULL,
						app );

	gmpv_mpv_obj_initialize(app->mpv);
	gmpv_mpv_obj_set_opengl_cb_callback
		(app->mpv, opengl_cb_update_callback, app);
	gmpv_mpv_obj_set_event_callback
		(app->mpv, mpv_event_handler, app);

	if(!app->files)
	{
		GmpvControlBox *control_box;

		control_box = gmpv_main_window_get_control_box(app->gui);

		gmpv_control_box_set_enabled(control_box, FALSE);
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

static void playlist_row_activated_handler(	GmpvPlaylistWidget *playlist,
						gint64 pos,
						gpointer data )
{
	set_playlist_pos(data, pos);
}

static void playlist_row_deleted_handler(	GmpvPlaylistWidget *playlist,
						gint pos,
						gpointer data )
{
	GmpvApplication *app = data;

	if(gmpv_mpv_obj_is_loaded(app->mpv))
	{
		const gchar *cmd[] = {"playlist_remove", NULL, NULL};
		gchar *index_str = g_strdup_printf("%d", pos);

		cmd[1] = index_str;

		mpv_check_error(gmpv_mpv_obj_command(app->mpv, cmd));

		g_free(index_str);
	}

	if(gmpv_playlist_empty(app->playlist_store))
	{
		GmpvControlBox *control_box;

		control_box = gmpv_main_window_get_control_box(app->gui);

		gmpv_control_box_set_enabled(control_box, FALSE);
	}
}

static void playlist_row_reodered_handler(	GmpvPlaylistWidget *playlist,
						gint src,
						gint dest,
						gpointer data )
{
	GmpvApplication *app = data;
	const gchar *cmd[] = {"playlist_move", NULL, NULL, NULL};
	gchar *src_str = g_strdup_printf("%d", (src > dest)?--src:src);
	gchar *dest_str = g_strdup_printf("%d", dest);

	cmd[1] = src_str;
	cmd[2] = dest_str;

	gmpv_mpv_obj_command(app->mpv, cmd);

	g_free(src_str);
	g_free(dest_str);
}

static GmpvTrack *parse_track_list(mpv_node_list *node)
{
	GmpvTrack *entry = gmpv_track_new();

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

static void update_track_list(GmpvApplication *app, mpv_node* track_list)
{
	const struct
	{
		const gchar *prop_name;
		const gchar *action_name;
	}
	track_map[] = {	{"aid", "audio_select"},
			{"vid", "video_select"},
			{"sid", "sub_select"},
			{NULL, NULL} };

	GmpvMpvObj *mpv = app->mpv;
	mpv_node_list *org_list = track_list->u.list;
	GSList *audio_list = NULL;
	GSList *video_list = NULL;
	GSList *sub_list = NULL;

	for(gint i = 0; track_map[i].prop_name; i++)
	{
		GAction *action;
		gchar *buf;
		gint64 val;

		action =	g_action_map_lookup_action
				(G_ACTION_MAP(app), track_map[i].action_name);
		buf =	gmpv_mpv_obj_get_property_string
			(mpv, track_map[i].prop_name);
		val = g_ascii_strtoll(buf, NULL, 10);

		g_simple_action_set_state
			(G_SIMPLE_ACTION(action), g_variant_new_int64(val));

		mpv_free(buf);
	}

	for(gint i = 0; i < org_list->num; i++)
	{
		GmpvTrack *entry = parse_track_list(org_list->values[i].u.list);
		GSList **list;

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
			g_assert_not_reached();
		}

		*list = g_slist_prepend(*list, entry);
	}

	audio_list = g_slist_reverse(audio_list);
	video_list = g_slist_reverse(video_list);
	sub_list = g_slist_reverse(sub_list);

	gmpv_main_window_update_track_list
		(app->gui, audio_list, video_list, sub_list);

	g_slist_free_full(audio_list, (GDestroyNotify)gmpv_track_free);
	g_slist_free_full(video_list, (GDestroyNotify)gmpv_track_free);
	g_slist_free_full(sub_list, (GDestroyNotify)gmpv_track_free);
}

static void mpv_prop_change_handler(mpv_event_property *prop, gpointer data)
{
	GmpvApplication *app = data;
	GmpvMpvObj *mpv;
	GmpvControlBox *control_box;
	GmpvMpvObjState state;

	mpv = app->mpv;
	control_box = gmpv_main_window_get_control_box(app->gui);

	gmpv_mpv_obj_get_state(mpv, &state);

	if(g_strcmp0(prop->name, "pause") == 0)
	{
		gboolean paused = prop->data?*((gboolean *)prop->data):TRUE;

		gmpv_control_box_set_playing_state(control_box, !paused);
	}
	else if(g_strcmp0(prop->name, "core-idle") == 0)
	{
		gboolean idle = prop->data?*((gboolean *)prop->data):TRUE;

		set_inhibit_idle(app, !idle);
	}
	else if(g_strcmp0(prop->name, "track-list") == 0 && prop->data)
	{
		update_track_list(app, prop->data);
	}
	else if(g_strcmp0(prop->name, "volume") == 0
	&& (state.init_load || state.loaded))
	{
		gdouble volume = prop->data?*((double *)prop->data)/100.0:0;

		gmpv_control_box_set_volume(control_box, volume);
	}
	else if(g_strcmp0(prop->name, "aid") == 0)
	{
		/* prop->data == NULL iff there is no audio track */
		gmpv_control_box_set_volume_enabled(control_box, !!prop->data);
	}
	else if(g_strcmp0(prop->name, "length") == 0 && prop->data)
	{
		gdouble length = *((gdouble *) prop->data);

		gmpv_control_box_set_seek_bar_length(control_box, (gint)length);
	}
	else if(g_strcmp0(prop->name, "media-title") == 0 && prop->data)
	{
		const gchar *title = *((const gchar **)prop->data);

		gtk_window_set_title(GTK_WINDOW(app->gui), title);
	}
	else if(g_strcmp0(prop->name, "playlist-pos") == 0 && prop->data)
	{
		GmpvPlaylist *playlist = gmpv_mpv_obj_get_playlist(mpv);
		gint64 pos = *((gint64 *)prop->data);

		gmpv_playlist_set_indicator_pos(playlist, (gint)pos);
	}
	else if(g_strcmp0(prop->name, "chapters") == 0 && prop->data)
	{
		gint64 count = *((gint64 *) prop->data);

		gmpv_control_box_set_chapter_enabled(control_box, (count > 1));
	}
}

static void mpv_event_handler(mpv_event *event, gpointer data)
{
	GmpvApplication *app = data;
	GmpvMpvObj *mpv = app->mpv;
	GmpvMpvObjState state;

	gmpv_mpv_obj_get_state(mpv, &state);

	if(event->event_id == MPV_EVENT_VIDEO_RECONFIG)
	{
		gdouble autofit_ratio = gmpv_mpv_obj_get_autofit_ratio(app->mpv);

		if(state.new_file && autofit_ratio > 0)
		{
			resize_window_to_fit(app, autofit_ratio);
		}
	}
	else if(event->event_id == MPV_EVENT_PROPERTY_CHANGE)
	{
		if(state.loaded)
		{
			mpv_prop_change_handler(event->data, data);
		}
	}
	else if(event->event_id == MPV_EVENT_FILE_LOADED)
	{
		GmpvControlBox *control_box;
		GmpvPlaylist *playlist;
		gchar *title;
		gchar *aid_str;
		gint64 aid;
		gint64 pos = -1;
		gdouble length = 0;

		control_box = gmpv_main_window_get_control_box(app->gui);
		playlist = gmpv_mpv_obj_get_playlist(mpv);

		if(app->target_playlist_pos != -1)
		{
			gmpv_mpv_obj_set_property
				(	mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&app->target_playlist_pos );

			app->target_playlist_pos = -1;
		}

		gmpv_mpv_obj_get_property
			(mpv, "playlist-pos", MPV_FORMAT_INT64, &pos);
		gmpv_mpv_obj_get_property
			(mpv, "length", MPV_FORMAT_DOUBLE, &length);

		aid_str = gmpv_mpv_obj_get_property_string(mpv, "aid");
		aid = g_ascii_strtoll(aid_str, NULL, 10);

		title = gmpv_mpv_obj_get_property_string(mpv, "media-title");

		gmpv_control_box_set_enabled(control_box, TRUE);
		gmpv_control_box_set_volume_enabled(control_box, (aid > 0));
		gmpv_control_box_set_playing_state(control_box, !state.paused);
		gmpv_playlist_set_indicator_pos(playlist, (gint)pos);
		gmpv_control_box_set_seek_bar_length(control_box, (gint)length);
		gtk_window_set_title(GTK_WINDOW(app->gui), title);

		mpv_free(aid_str);
		mpv_free(title);
	}
	else if(event->event_id == MPV_EVENT_CLIENT_MESSAGE)
	{
		mpv_event_client_message *event_cmsg = event->data;

		if(event_cmsg->num_args == 2
		&& g_strcmp0(event_cmsg->args[0], "gmpv-action") == 0)
		{
			activate_action_string(app, event_cmsg->args[1]);
		}
		else
		{
			gint num_args = event_cmsg->num_args;
			gsize args_size = ((gsize)num_args+1)*sizeof(gchar *);
			gchar **args = g_malloc(args_size);
			gchar *full_str = NULL;

			/* Concatenate arguments into one string */
			memcpy(args, event_cmsg->args, args_size);

			args[num_args] = NULL;
			full_str = g_strjoinv(" ", args);

			g_warning(	"Invalid client message received: %s",
					full_str );

			g_free(full_str);
		}
	}
	else if(event->event_id == MPV_EVENT_IDLE)
	{
		if(!state.init_load && !state.loaded)
		{
			gmpv_main_window_reset(app->gui);
			set_inhibit_idle(app, FALSE);
		}
	}
	else if(event->event_id == MPV_EVENT_SHUTDOWN)
	{
		quit(app);
	}
}

static void mpv_error_handler(GmpvMpvObj *mpv, const gchar *err, gpointer data)
{
	GmpvApplication *app = data;

	gmpv_main_window_reset(app->gui);
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
	GmpvApplication *app = data;
	gchar *type = gdk_atom_name(gtk_selection_data_get_target(sel_data));
	const guchar *raw_data = gtk_selection_data_get_data(sel_data);
	gchar **uri_list = gtk_selection_data_get_uris(sel_data);
	gboolean append = GMPV_IS_PLAYLIST_WIDGET(widget);

	if(g_strcmp0(type, "PLAYLIST_PATH") == 0)
	{
		GtkTreePath *path =	gtk_tree_path_new_from_string
					((const gchar *)raw_data);

		g_assert(path);
		set_playlist_pos(app, gtk_tree_path_get_indices(path)[0]);

		gtk_tree_path_free(path);
	}
	else
	{
		if(uri_list)
		{
			gmpv_mpv_obj_load_list(	app->mpv,
						(const gchar **)uri_list,
						append,
						TRUE );
		}
		else
		{
			gmpv_mpv_obj_load(	app->mpv,
						(const gchar *)raw_data,
						append,
						TRUE );
		}
	}

	g_strfreev(uri_list);
	g_free(type);
}

static gchar *get_full_keystr(guint keyval, guint state)
{
	/* strlen("Ctrl+Alt+Shift+Meta+")+1 == 21 */
	const gsize max_modstr_len = 21;
	gchar modstr[max_modstr_len];
	gboolean found = FALSE;
	const gchar *keystr = gdk_keyval_name(keyval);
	const gchar *keystrmap[] = KEYSTRING_MAP;
	modstr[0] = '\0';

	if((state&GDK_SHIFT_MASK) != 0)
	{
		g_strlcat(modstr, "Shift+", max_modstr_len);
	}

	if((state&GDK_CONTROL_MASK) != 0)
	{
		g_strlcat(modstr, "Ctrl+", max_modstr_len);
	}

	if((state&GDK_MOD1_MASK) != 0)
	{
		g_strlcat(modstr, "Alt+", max_modstr_len);
	}

	if((state&GDK_META_MASK) != 0 || (state&GDK_SUPER_MASK) != 0)
	{
		g_strlcat(modstr, "Meta+", max_modstr_len);
	}

	/* Translate GDK key name to mpv key name */
	for(gint i = 0; !found && keystrmap[i]; i += 2)
	{
		gint rc = g_ascii_strncasecmp(	keystr,
						keystrmap[i+1],
						KEYSTRING_MAX_LEN );

		if(rc == 0)
		{
			keystr = keystrmap[i];
			found = TRUE;
		}
	}

	return (strlen(keystr) > 0)?g_strconcat(modstr, keystr, NULL):NULL;
}

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	const gchar *cmd[] = {"keydown", NULL, NULL};
	GmpvApplication *app = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = NULL;

	cmd[1] = keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		g_debug("Sent '%s' key down to mpv", keystr);
		gmpv_mpv_obj_command(app->mpv, cmd);

		g_free(keystr);
	}

	return FALSE;
}

static gboolean key_release_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvApplication *app = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = NULL;
	const gchar *cmd[] = {"keyup", NULL, NULL};

	cmd[1] = keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		g_debug("Sent '%s' key up to mpv", keystr);
		gmpv_mpv_obj_command(app->mpv, cmd);

		g_free(keystr);
	}

	return FALSE;
}

static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GmpvApplication *app = data;
	GdkEventButton *btn_event = (GdkEventButton *)event;
	gchar *x_str = g_strdup_printf("%d", (gint)btn_event->x);
	gchar *y_str = g_strdup_printf("%d", (gint)btn_event->y);
	gchar *btn_str = g_strdup_printf("%u", btn_event->button-1);
	const gchar *type_str =	(btn_event->type == GDK_2BUTTON_PRESS)?
				"double":"single";

	const gchar *cmd[] = {"mouse", x_str, y_str, btn_str, type_str, NULL};

	g_debug(	"Sent %s button %s click at %sx%s to mpv",
			type_str, btn_str, x_str, y_str );

	gmpv_mpv_obj_command(app->mpv, cmd);

	g_free(x_str);
	g_free(y_str);
	g_free(btn_str);

	return TRUE;
}

static gboolean scroll_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	GdkEventScroll *scroll_event = (GdkEventScroll *)event;
	guint button = 0;
	gint count = 0;

	/* Only one axis will be used at a time to prevent accidental activation
	 * of commands bound to buttons associated with the other axis.
	 */
	if(ABS(scroll_event->delta_x) > ABS(scroll_event->delta_y))
	{
		count = (gint)ABS(scroll_event->delta_x);

		if(scroll_event->delta_x <= -1)
		{
			button = 6;
		}
		else if(scroll_event->delta_x >= 1)
		{
			button = 7;
		}
	}
	else
	{
		count = (gint)ABS(scroll_event->delta_y);

		if(scroll_event->delta_y <= -1)
		{
			button = 4;
		}
		else if(scroll_event->delta_y >= 1)
		{
			button = 5;
		}
	}

	if(button > 0)
	{
		GdkEventButton btn_event;

		btn_event.type = scroll_event->type;
		btn_event.window = scroll_event->window;
		btn_event.send_event = scroll_event->send_event;
		btn_event.time = scroll_event->time;
		btn_event.x = scroll_event->x;
		btn_event.y = scroll_event->y;
		btn_event.axes = NULL;
		btn_event.state = scroll_event->state;
		btn_event.button = button;
		btn_event.device = scroll_event->device;
		btn_event.x_root = scroll_event->x_root;
		btn_event.y_root = scroll_event->y_root;

		for(gint i = 0; i < count; i++)
		{
			/* Not used */
			gboolean rc;

			g_signal_emit_by_name
				(widget, "button-press-event", &btn_event, &rc);
		}
	}

	return TRUE;
}

static gboolean queue_render(GtkGLArea *area)
{
	gtk_gl_area_queue_render(area);

	return FALSE;
}

static void opengl_cb_update_callback(void *cb_ctx)
{
	GmpvApplication *app = cb_ctx;
	GtkGLArea *area =	gmpv_video_area_get_gl_area
				(gmpv_main_window_get_video_area(app->gui));

	g_idle_add_full(	G_PRIORITY_HIGH,
				(GSourceFunc)queue_render,
				area,
				NULL );
}

static void set_playlist_pos(GmpvApplication *app, gint64 pos)
{
	gint64 mpv_pos;
	gint rc;

	rc = gmpv_mpv_obj_get_property(	app->mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&mpv_pos );

	gmpv_mpv_obj_set_property_flag(app->mpv, "pause", FALSE);

	if(pos != mpv_pos)
	{
		if(rc >= 0)
		{
			gmpv_mpv_obj_set_property(	app->mpv,
							"playlist-pos",
							MPV_FORMAT_INT64,
							&pos );
		}
		else
		{
			app->target_playlist_pos = pos;
		}
	}
}

static void set_inhibit_idle(GmpvApplication *app, gboolean inhibit)
{
	if(inhibit && app->inhibit_cookie == 0)
	{
		app->inhibit_cookie
			= gtk_application_inhibit
				(	GTK_APPLICATION(app),
					GTK_WINDOW(app->gui),
					GTK_APPLICATION_INHIBIT_IDLE,
					_("Playing") );
	}
	else if(!inhibit && app->inhibit_cookie != 0)
	{
		gtk_application_uninhibit
			(GTK_APPLICATION(app), app->inhibit_cookie);

		app->inhibit_cookie = 0;
	}
}

static gboolean load_files(GmpvApplication *app, const gchar **files)
{
	GmpvMpvObjState state;

	gmpv_mpv_obj_get_state(app->mpv, &state);

	if(files)
	{
		gmpv_mpv_obj_load_list(app->mpv, files, FALSE, TRUE);

		g_strfreev(app->files);

		app->files = NULL;
	}

	return FALSE;
}

static void connect_signals(GmpvApplication *app)
{
	GmpvPlaylistWidget *playlist;
	GmpvVideoArea *video_area;

	playlist = gmpv_main_window_get_playlist(app->gui);
	video_area = gmpv_main_window_get_video_area(app->gui);

	gmpv_playbackctl_connect_signals(app);

	g_signal_connect_after(	app->gui,
				"draw",
				G_CALLBACK(draw_handler),
				app );
	g_signal_connect(	app->mpv,
				"mpv-init",
				G_CALLBACK(mpv_init_handler),
				app );
	g_signal_connect(	video_area,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				app );
	g_signal_connect(	playlist,
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
	g_signal_connect(	app->gui,
				"key-release-event",
				G_CALLBACK(key_release_handler),
				app );
	g_signal_connect(	video_area,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
				app );
	g_signal_connect(	video_area,
				"scroll-event",
				G_CALLBACK(scroll_handler),
				app );
	g_signal_connect(	playlist,
				"row-activated",
				G_CALLBACK(playlist_row_activated_handler),
				app );
	g_signal_connect(	playlist,
				"row-deleted",
				G_CALLBACK(playlist_row_deleted_handler),
				app );
	g_signal_connect(	playlist,
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

static void setup_accelerators(GmpvApplication *app)
{
	GtkApplication *gtk_app = GTK_APPLICATION(app);

	add_accelerator(gtk_app, "<Control>o", "app.open(false)");
	add_accelerator(gtk_app, "<Control>l", "app.openloc");
	add_accelerator(gtk_app, "<Control>s", "app.playlist_save");
	add_accelerator(gtk_app, "<Control>q", "app.quit");
	add_accelerator(gtk_app, "<Control>question", "app.show_shortcuts");
	add_accelerator(gtk_app, "<Control>p", "app.pref");
	add_accelerator(gtk_app, "<Control>1", "app.video_size(@d 1)");
	add_accelerator(gtk_app, "<Control>2", "app.video_size(@d 2)");
	add_accelerator(gtk_app, "<Control>3", "app.video_size(@d 0.5)");
	add_accelerator(gtk_app, "F9", "app.playlist_toggle");
	add_accelerator(gtk_app, "F11", "app.fullscreen_toggle");
}


static void gmpv_application_class_init(GmpvApplicationClass *klass)
{
}

static void gmpv_application_init(GmpvApplication *app)
{
	g_signal_connect(app, "startup", G_CALLBACK(startup_handler), app);
	g_signal_connect(app, "activate", G_CALLBACK(activate_handler), app);
	g_signal_connect(app, "open", G_CALLBACK(open_handler), app);
}

GmpvMainWindow *gmpv_application_get_main_window(GmpvApplication *app)
{
	return app->gui;
}

GmpvMpvObj *gmpv_application_get_mpv_obj(GmpvApplication *app)
{
	return app->mpv;
}

GmpvApplication *gmpv_application_new(gchar *id, GApplicationFlags flags)
{
	return GMPV_APPLICATION(g_object_new(	gmpv_application_get_type(),
						"application-id", id,
						"flags", flags,
						NULL ));
}
