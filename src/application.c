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

#include "application.h"
#include "control_box.h"
#include "actionctl.h"
#include "playbackctl.h"
#include "playlist_widget.h"
#include "track.h"
#include "menu.h"
#include "mpv_obj.h"
#include "video_area.h"
#include "def.h"
#include "mpris/mpris.h"
#include "media_keys/media_keys.h"

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
static void playlist_row_activated_handler(	PlaylistWidget *playlist,
						gint64 pos,
						gpointer data );
static void playlist_row_deleted_handler(	PlaylistWidget *playlist,
						gint pos,
						gpointer data );
static void playlist_row_reodered_handler(	PlaylistWidget *playlist,
						gint src,
						gint dest,
						gpointer data );
static Track *parse_track_list(mpv_node_list *node);
static void update_track_list(Application *app, mpv_node* track_list);
static void mpv_prop_change_handler(mpv_event_property *prop, gpointer data);
static void mpv_event_handler(mpv_event *event, gpointer data);
static void mpv_error_handler(MpvObj *mpv, const gchar *err, gpointer data);
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
static gboolean queue_render(GtkGLArea *area);
static void opengl_cb_update_callback(void *cb_ctx);
static void set_playlist_pos(Application *app, gint64 pos);
static void set_inhibit_idle(Application *app, gboolean inhibit);
static gboolean load_files(Application* app, const gchar **files);
static void connect_signals(Application *app);
static inline void add_accelerator(	GtkApplication *app,
					const char *accel,
					const char *action );
static void setup_accelerators(Application *app);
static void application_class_init(ApplicationClass *klass);
static void application_init(Application *app);

G_DEFINE_TYPE(Application, application, GTK_TYPE_APPLICATION)

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

	if(app->mpv->opengl_ctx)
	{
		int width;
		int height;
		int fbo;

		width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
		height = (-1)*gtk_widget_get_allocated_height(GTK_WIDGET(area));
		fbo = -1;

		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
		mpv_opengl_cb_draw(app->mpv->opengl_ctx, fbo, width, height);
	}

	while(gtk_events_pending())
	{
		gtk_main_iteration();
	}

	return TRUE;
}

static void startup_handler(GApplication *gapp, gpointer data)
{
	Application *app = data;
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
	app->playlist_store = playlist_new();
	app->gui = MAIN_WINDOW(main_window_new(app, app->playlist_store));
	app->fs_control = NULL;

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

	setup_accelerators(app);
	actionctl_map_actions(app);
	main_window_load_state(app->gui);
	gtk_widget_show_all(GTK_WIDGET(app->gui));

	if(csd_enable)
	{
		control_box_set_fullscreen_btn_visible
			(CONTROL_BOX(app->gui->control_box), FALSE);
	}

	control_box_set_chapter_enabled
		(CONTROL_BOX(app->gui->control_box), FALSE);

	wid = video_area_get_xid(VIDEO_AREA(app->gui->vid_area));
	app->mpv = mpv_obj_new(app->playlist_store, wid);

	if(csd_enable)
	{
		control_box_set_fullscreen_btn_visible
			(CONTROL_BOX(app->gui->control_box), FALSE);
	}

	control_box_set_chapter_enabled
		(CONTROL_BOX(app->gui->control_box), FALSE);

	connect_signals(app);
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

static void mpv_init_handler(MpvObj *mpv, gpointer data)
{
	Application *app = data;
	gchar *current_vo = mpv_obj_get_property_string(mpv, "current-vo");
	VideoArea *vid_area = VIDEO_AREA(app->gui->vid_area);

	video_area_set_use_opengl(vid_area, !current_vo);

	/* current_vo should be NULL if the selected vo is opengl-cb */
	if(!current_vo)
	{
		GtkGLArea *gl_area;
		gint rc;

		gl_area = video_area_get_gl_area(vid_area);
		g_signal_connect(	gl_area,
					"render",
					G_CALLBACK(vid_area_render_handler),
					app );

		gtk_gl_area_make_current(gl_area);
		rc = mpv_opengl_cb_init_gl(	mpv->opengl_ctx,
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
	Application *app = data;
	gint i;

	if(n_files > 0)
	{
		MpvObjState state;

		app->files = g_malloc(sizeof(GFile *)*(gsize)(n_files+1));

		for(i = 0; i < n_files; i++)
		{
			app->files[i] = g_file_get_uri(((GFile **)files)[i]);
		}

		app->files[i] = NULL;

		mpv_obj_get_state(app->mpv, &state);

		if(state.ready)
		{
			load_files(app, (const gchar **)app->files);
		}
	}

	gdk_notify_startup_complete();
}

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

	mpv_obj_initialize(app->mpv);
	mpv_obj_set_opengl_cb_callback(app->mpv, opengl_cb_update_callback, app);
	mpv_obj_set_wakup_callback(app->mpv, mpv_obj_wakeup_callback, app);

	if(!app->files)
	{
		control_box_set_enabled
			(CONTROL_BOX(app->gui->control_box), FALSE);
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

static void playlist_row_activated_handler(	PlaylistWidget *playlist,
						gint64 pos,
						gpointer data )
{
	set_playlist_pos(data, pos);
}

static void playlist_row_deleted_handler(	PlaylistWidget *playlist,
						gint pos,
						gpointer data )
{
	Application *app = data;

	if(mpv_obj_is_loaded(app->mpv))
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

static void playlist_row_reodered_handler(	PlaylistWidget *playlist,
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

	mpv_obj_get_property(	mpv,
				"aid",
				MPV_FORMAT_INT64,
				&aid );
	mpv_obj_get_property(	mpv,
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

	g_slist_free_full(audio_list, (GDestroyNotify)track_free);
	g_slist_free_full(video_list, (GDestroyNotify)track_free);
	g_slist_free_full(sub_list, (GDestroyNotify)track_free);
}

static void mpv_prop_change_handler(mpv_event_property *prop, gpointer data)
{
	Application *app = data;
	MpvObj *mpv = app->mpv;
	ControlBox *control_box = CONTROL_BOX(app->gui->control_box);
	MpvObjState state;

	mpv_obj_get_state(mpv, &state);

	if(g_strcmp0(prop->name, "pause") == 0)
	{
		gboolean paused = prop->data?*((gboolean *)prop->data):TRUE;

		control_box_set_playing_state(control_box, !paused);
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

		control_box_set_volume(control_box, volume);
	}
	else if(g_strcmp0(prop->name, "aid") == 0)
	{
		/* prop->data == NULL iff there is no audio track */
		control_box_set_volume_enabled(control_box, !!prop->data);
	}
	else if(g_strcmp0(prop->name, "length") == 0 && prop->data)
	{
		gdouble length = *((gdouble *) prop->data);

		control_box_set_seek_bar_length(control_box, (gint)length);
	}
	else if(g_strcmp0(prop->name, "media-title") == 0 && prop->data)
	{
		const gchar *title = *((const gchar **)prop->data);

		gtk_window_set_title(GTK_WINDOW(app->gui), title);
	}
	else if(g_strcmp0(prop->name, "playlist-pos") == 0 && prop->data)
	{
		Playlist *playlist = mpv_obj_get_playlist(mpv);
		gint64 pos = *((gint64 *)prop->data);

		playlist_set_indicator_pos(playlist, (gint)pos);
	}
	else if(g_strcmp0(prop->name, "chapters") == 0 && prop->data)
	{
		gint64 count = *((gint64 *) prop->data);

		control_box_set_chapter_enabled(control_box, (count > 1));
	}
}

static void mpv_event_handler(mpv_event *event, gpointer data)
{
	Application *app = data;
	MpvObj *mpv = app->mpv;
	MpvObjState state;

	mpv_obj_get_state(mpv, &state);

	if(event->event_id == MPV_EVENT_VIDEO_RECONFIG)
	{
		gdouble autofit_ratio = mpv_obj_get_autofit_ratio(app->mpv);

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
		ControlBox *control_box = CONTROL_BOX(app->gui->control_box);
		Playlist *playlist = mpv_obj_get_playlist(mpv);
		gint64 aid = -1;
		gint64 pos = -1;
		gdouble length = 0;
		gchar *title = NULL;

		if(app->target_playlist_pos != -1)
		{
			mpv_obj_set_property(	mpv,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&app->target_playlist_pos );

			app->target_playlist_pos = -1;
		}

		mpv_obj_get_property
			(mpv, "aid", MPV_FORMAT_INT64, &aid);
		mpv_obj_get_property
			(mpv, "playlist-pos", MPV_FORMAT_INT64, &pos);
		mpv_obj_get_property
			(mpv, "length", MPV_FORMAT_DOUBLE, &length);

		title = mpv_obj_get_property_string(mpv, "media-title");

		control_box_set_enabled(control_box, TRUE);
		control_box_set_volume_enabled(control_box, (aid != -1));
		control_box_set_playing_state(control_box, !state.paused);
		playlist_set_indicator_pos(playlist, (gint)pos);
		control_box_set_seek_bar_length(control_box, (gint)length);
		gtk_window_set_title(GTK_WINDOW(app->gui), title);

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
	Application *app = data;
	gchar *type = gdk_atom_name(gtk_selection_data_get_target(sel_data));
	const guchar *raw_data = gtk_selection_data_get_data(sel_data);
	gchar **uri_list = gtk_selection_data_get_uris(sel_data);
	gboolean append = IS_PLAYLIST_WIDGET(widget);

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
			mpv_obj_load_list(	app->mpv,
						(const gchar **)uri_list,
						append,
						TRUE );
		}
		else
		{
			mpv_obj_load(	app->mpv,
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
	Application *app = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = NULL;

	cmd[1] = keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		g_debug("Sent '%s' key down to mpv", keystr);
		mpv_obj_command(app->mpv, cmd);

		g_free(keystr);
	}

	return FALSE;
}

static gboolean key_release_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	Application *app = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;
	gchar *keystr = NULL;
	const gchar *cmd[] = {"keyup", NULL, NULL};

	cmd[1] = keystr = get_full_keystr(keyval, state);

	if(keystr)
	{
		g_debug("Sent '%s' key up to mpv", keystr);
		mpv_obj_command(app->mpv, cmd);

		g_free(keystr);
	}

	return FALSE;
}

static gboolean mouse_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	Application *app = data;
	GdkEventButton *btn_event = (GdkEventButton *)event;
	gchar *x_str = g_strdup_printf("%d", (gint)btn_event->x);
	gchar *y_str = g_strdup_printf("%d", (gint)btn_event->y);
	gchar *btn_str = g_strdup_printf("%u", btn_event->button-1);
	const gchar *type_str =	(btn_event->type == GDK_2BUTTON_PRESS)?
				"double":"single";

	const gchar *cmd[] = {"mouse", x_str, y_str, btn_str, type_str, NULL};

	g_debug(	"Sent %s button %s click at %sx%s to mpv",
			type_str, btn_str, x_str, y_str );

	mpv_obj_command(app->mpv, cmd);

	g_free(x_str);
	g_free(y_str);
	g_free(btn_str);

	return TRUE;
}

static gboolean queue_render(GtkGLArea *area)
{
	gtk_gl_area_queue_render(area);

	return FALSE;
}

static void opengl_cb_update_callback(void *cb_ctx)
{
	GtkGLArea *area =	video_area_get_gl_area
				(VIDEO_AREA(APPLICATION(cb_ctx)->gui->vid_area));

	g_idle_add_full(	G_PRIORITY_HIGH,
				(GSourceFunc)queue_render,
				area,
				NULL );
}

static void set_playlist_pos(Application *app, gint64 pos)
{
	gint64 mpv_pos;
	gint rc;

	rc = mpv_obj_get_property(	app->mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&mpv_pos );

	mpv_obj_set_property_flag(app->mpv, "pause", FALSE);

	if(pos != mpv_pos)
	{
		if(rc >= 0)
		{
			mpv_obj_set_property(	app->mpv,
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

static void set_inhibit_idle(Application *app, gboolean inhibit)
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

static gboolean load_files(Application *app, const gchar **files)
{
	MpvObjState state;

	mpv_obj_get_state(app->mpv, &state);

	if(files)
	{
		mpv_obj_load_list(app->mpv, files, FALSE, TRUE);

		g_strfreev(app->files);

		app->files = NULL;
	}

	return FALSE;
}

static void connect_signals(Application *app)
{
	PlaylistWidget *playlist = PLAYLIST_WIDGET(app->gui->playlist);

	playbackctl_connect_signals(app);

	g_signal_connect_after(	app->gui,
				"draw",
				G_CALLBACK(draw_handler),
				app );
	g_signal_connect(	app->mpv,
				"mpv-init",
				G_CALLBACK(mpv_init_handler),
				app );
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
	g_signal_connect(	app->gui,
				"key-release-event",
				G_CALLBACK(key_release_handler),
				app );
	g_signal_connect(	app->gui->vid_area,
				"button-press-event",
				G_CALLBACK(mouse_press_handler),
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

static void setup_accelerators(Application *app)
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
						NULL ));
}
