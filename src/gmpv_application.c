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
#include <gdk/gdk.h>
#include <locale.h>
#include <epoxy/gl.h>

#include "gmpv_application.h"
#include "gmpv_control_box.h"
#include "gmpv_actionctl.h"
#include "gmpv_inputctl.h"
#include "gmpv_playbackctl.h"
#include "gmpv_playlist_widget.h"
#include "gmpv_track.h"
#include "gmpv_common.h"
#include "gmpv_menu.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_video_area.h"
#include "gmpv_def.h"
#include "mpris/gmpv_mpris.h"
#include "media_keys/gmpv_media_keys.h"

struct _GmpvApplication
{
	GtkApplication parent;
	gboolean no_existing_session;
	GmpvMpv *mpv;
	GQueue *action_queue;
	gchar **files;
	guint inhibit_cookie;
	gint64 target_playlist_pos;
	GmpvMainWindow *gui;
	GtkWidget *fs_control;
	GmpvPlaylist *playlist_store;
};

struct _GmpvApplicationClass
{
	GtkApplicationClass parent_class;
};

static gboolean vid_area_render_handler(	GtkGLArea *area,
						GdkGLContext *context,
						gpointer data );
static gint options_handler(	GApplication *gapp,
				GVariantDict *options,
				gpointer data );
static gint command_line_handler(	GApplication *app,
					GApplicationCommandLine *cli,
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
static void grab_handler(GtkWidget *widget, gboolean was_grabbed, gpointer data);
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
static gboolean process_action(gpointer data);
static void mpv_prop_change_handler(	GmpvMpv* mpv,
					const gchar *prop,
					gpointer value,
					gpointer data );
static void load_handler(GmpvMpv *mpv, gpointer data);
static void idle_handler(GmpvMpv *mpv, gpointer data);
static void message_handler(GmpvMpv *mpv, const gchar *msg, gpointer data);
static void video_reconfig_handler(GmpvMpv *mpv, gpointer data);
static void shutdown_handler(GmpvMpv *mpv, gpointer data);
static void mpv_error_handler(GmpvMpv *mpv, const gchar *err, gpointer data);
static gboolean shutdown_signal_handler(gpointer data);
static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data);
static gboolean window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static gboolean queue_render(GtkGLArea *area);
static void opengl_cb_update_callback(void *cb_ctx);
static void set_playlist_pos(GmpvApplication *app, gint64 pos);
static void set_inhibit_idle(GmpvApplication *app, gboolean inhibit);
static gboolean load_files(	GmpvApplication* app,
				const gchar **files,
				gboolean append );
static void connect_signals(GmpvApplication *app);
static void gmpv_application_class_init(GmpvApplicationClass *klass);
static void gmpv_application_init(GmpvApplication *app);

G_DEFINE_TYPE(GmpvApplication, gmpv_application, GTK_TYPE_APPLICATION)

static gboolean vid_area_render_handler(	GtkGLArea *area,
						GdkGLContext *context,
						gpointer data )
{
	GmpvApplication *app = data;
	mpv_opengl_cb_context *opengl_ctx;

	opengl_ctx = gmpv_mpv_get_opengl_cb_context(app->mpv);

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
		GmpvApplication *app = data;
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

	return -1;
}

static gint command_line_handler(	GApplication *app,
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
		g_application_open(app, files, n_files, enqueue?"enqueue":"");
	}

	for(gint i = 0; i < n_files; i++)
	{
		g_object_unref(files[i]);
	}

	g_strfreev(argv);

	return 0;
}

static void startup_handler(GApplication *gapp, gpointer data)
{
	GmpvApplication *app = data;
	GmpvControlBox *control_box;
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	const gchar *style = ".gmpv-vid-area{background-color: black}";
	GtkCssProvider *style_provider;
	gboolean css_loaded;
	gboolean csd_enable;
	gboolean dark_theme_enable;
	gboolean always_floating;
	gint64 wid;
	gchar *mpvinput;

	setlocale(LC_NUMERIC, "C");
	g_set_application_name(_("GNOME MPV"));
	gtk_window_set_default_icon_name(ICON_NAME);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	g_info("Starting GNOME MPV " VERSION);

	csd_enable = g_settings_get_boolean
				(settings, "csd-enable");
	dark_theme_enable = g_settings_get_boolean
				(settings, "dark-theme-enable");
	always_floating = g_settings_get_boolean
				(settings, "always-use-floating-controls");
	mpvinput = g_settings_get_string
				(settings, "mpv-input-config-file");

	app->files = NULL;
	app->inhibit_cookie = 0;
	app->target_playlist_pos = -1;
	app->playlist_store = gmpv_playlist_new();
	app->gui = GMPV_MAIN_WINDOW(gmpv_main_window_new(	app,
								app->playlist_store,
								always_floating));
	app->fs_control = NULL;

	migrate_config(app);

	control_box = gmpv_main_window_get_control_box(app->gui);
	style_provider = gtk_css_provider_new();
	css_loaded =	gtk_css_provider_load_from_data
			(style_provider, style, -1, NULL);

	if(!css_loaded)
	{
		g_warning ("Failed to apply background color css");
	}

	gtk_style_context_add_provider_for_screen
		(	gtk_window_get_screen(GTK_WINDOW(app->gui)),
			GTK_STYLE_PROVIDER(style_provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION );

	g_object_unref(style_provider);

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

	gmpv_actionctl_map_actions(app);
	gmpv_main_window_load_state(app->gui);
	gtk_widget_show_all(GTK_WIDGET(app->gui));

	if(csd_enable)
	{
		gmpv_control_box_set_fullscreen_btn_visible(control_box, FALSE);
	}

	gmpv_control_box_set_chapter_enabled(control_box, FALSE);

	wid = gmpv_video_area_get_xid(gmpv_main_window_get_video_area(app->gui));
	app->mpv = gmpv_mpv_new(app->playlist_store, wid);

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

static void mpv_init_handler(GmpvMpv *mpv, gpointer data)
{
	GmpvApplication *app = data;
	gchar *current_vo = gmpv_mpv_get_property_string(mpv, "current-vo");
	GmpvVideoArea *vid_area = gmpv_main_window_get_video_area(app->gui);

	gmpv_video_area_set_use_opengl(vid_area, !current_vo);

	/* current_vo should be NULL if the selected vo is opengl-cb */
	if(!current_vo)
	{
		GtkGLArea *gl_area = gmpv_video_area_get_gl_area(vid_area);

		g_signal_connect(	gl_area,
					"render",
					G_CALLBACK(vid_area_render_handler),
					app );

		gtk_gl_area_make_current(gl_area);
		gmpv_mpv_init_gl(app->mpv);
	}

	mpv_free(current_vo);
	load_files(app, (const gchar **)app->files, FALSE);
}

static void open_handler(	GApplication *gapp,
				gpointer files,
				gint n_files,
				gchar *hint,
				gpointer data )
{
	GmpvApplication *app = data;
	gboolean append = (g_strcmp0(hint, "enqueue") == 0);
	gint i;

	if(n_files > 0)
	{
		app->files = g_malloc(sizeof(GFile *)*(gsize)(n_files+1));

		for(i = 0; i < n_files; i++)
		{
			app->files[i] = g_file_get_uri(((GFile **)files)[i]);
		}

		app->files[i] = NULL;

		if(gmpv_mpv_get_state(app->mpv)->ready)
		{
			load_files(app, (const gchar **)app->files, append);
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

	gmpv_mpv_initialize(app->mpv);
	gmpv_main_window_set_geometry
		(app->gui, gmpv_mpv_get_geometry(app->mpv));
	gmpv_mpv_set_opengl_cb_callback
		(app->mpv, opengl_cb_update_callback, app);

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
	gmpv_application_quit(data);

	return TRUE;
}

static void grab_handler(GtkWidget *widget, gboolean was_grabbed, gpointer data)
{
	GmpvApplication *app = data;

	if(!was_grabbed)
	{
		g_debug(	"Main window has been shadowed; "
				"sending global keyup to mpv" );

		gmpv_mpv_command_string(app->mpv, "keyup");
	}
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

	if(gmpv_mpv_get_state(app->mpv)->loaded)
	{
		const gchar *cmd[] = {"playlist_remove", NULL, NULL};
		gchar *index_str = g_strdup_printf("%d", pos);

		cmd[1] = index_str;

		gmpv_mpv_command(app->mpv, cmd);

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

	gmpv_mpv_command(app->mpv, cmd);

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
	track_map[] = {	{"aid", "set-audio-track"},
			{"vid", "set-video-track"},
			{"sid", "set-subtitle-track"},
			{NULL, NULL} };

	GmpvMpv *mpv = app->mpv;
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
		buf =	gmpv_mpv_get_property_string
			(mpv, track_map[i].prop_name);
		val = g_ascii_strtoll(buf, NULL, 10);

		g_simple_action_set_state
			(G_SIMPLE_ACTION(action), g_variant_new_int64(val));

		gmpv_mpv_free(buf);
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

static gboolean process_action(gpointer data)
{
	GmpvApplication *app = data;
	gchar *action_str = g_queue_pop_tail(app->action_queue);

	activate_action_string(app, action_str);

	g_free(action_str);

	return FALSE;
}

static void mpv_prop_change_handler(	GmpvMpv *mpv,
					const gchar *prop,
					gpointer value,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvControlBox *control_box = gmpv_main_window_get_control_box(app->gui);
	const GmpvMpvState *state = gmpv_mpv_get_state(mpv);

	if(state->loaded)
	{
		if(g_strcmp0(prop, "pause") == 0)
		{
			gboolean paused = value?*((gboolean *)value):TRUE;

			gmpv_control_box_set_playing_state(control_box, !paused);
		}
		else if(g_strcmp0(prop, "core-idle") == 0)
		{
			gboolean idle = value?*((gboolean *)value):TRUE;

			set_inhibit_idle(app, !idle);
		}
		else if(g_strcmp0(prop, "track-list") == 0 && value)
		{
			update_track_list(app, value);
		}
		else if(g_strcmp0(prop, "volume") == 0
		&& (state->init_load || state->loaded))
		{
			gdouble volume = value?*((double *)value)/100.0:0;

			gmpv_control_box_set_volume(control_box, volume);
		}
		else if(g_strcmp0(prop, "duration") == 0 && value)
		{
			gdouble duration = *((gdouble *) value);

			gmpv_control_box_set_seek_bar_duration
				(control_box, (gint)duration);
		}
		else if(g_strcmp0(prop, "media-title") == 0 && value)
		{
			const gchar *title = *((const gchar **)value);

			gtk_window_set_title(GTK_WINDOW(app->gui), title);
		}
		else if(g_strcmp0(prop, "playlist-pos") == 0 && value)
		{
			GmpvPlaylist *playlist = gmpv_mpv_get_playlist(mpv);
			gint64 pos = *((gint64 *)value);

			gmpv_playlist_set_indicator_pos(playlist, (gint)pos);
		}
		else if(g_strcmp0(prop, "chapters") == 0 && value)
		{
			gint64 count = *((gint64 *) value);

			gmpv_control_box_set_chapter_enabled
				(control_box, (count > 1));
		}
		else if(g_strcmp0(prop, "fullscreen") == 0 && value)
		{
			gboolean fullscreen = *((gboolean *)value);

			gmpv_main_window_set_fullscreen(app->gui, fullscreen);
		}
	}
}

static void load_handler(GmpvMpv *mpv, gpointer data)
{
	GmpvApplication *app = data;
	const GmpvMpvState *state = gmpv_mpv_get_state(mpv);
	GmpvControlBox *control_box;
	GmpvPlaylist *playlist;
	gchar *title;
	gint64 pos = -1;
	gdouble duration = 0;

	control_box = gmpv_main_window_get_control_box(app->gui);
	playlist = gmpv_mpv_get_playlist(mpv);

	if(app->target_playlist_pos != -1)
	{
		gmpv_mpv_set_property
			(	mpv,
				"playlist-pos",
				MPV_FORMAT_INT64,
				&app->target_playlist_pos );

		app->target_playlist_pos = -1;
	}

	gmpv_mpv_get_property
		(mpv, "playlist-pos", MPV_FORMAT_INT64, &pos);
	gmpv_mpv_get_property
		(mpv, "duration", MPV_FORMAT_DOUBLE, &duration);

	title = gmpv_mpv_get_property_string(mpv, "media-title");

	gmpv_control_box_set_enabled(control_box, TRUE);
	gmpv_control_box_set_playing_state(control_box, !state->paused);
	gmpv_playlist_set_indicator_pos(playlist, (gint)pos);
	gmpv_control_box_set_seek_bar_duration(control_box, (gint)duration);
	gtk_window_set_title(GTK_WINDOW(app->gui), title);

	gmpv_mpv_free(title);
}

static void idle_handler(GmpvMpv *mpv, gpointer data)
{
	GmpvApplication *app = data;

	gmpv_main_window_reset(app->gui);
	set_inhibit_idle(app, FALSE);
}

static void message_handler(GmpvMpv *mpv, const gchar *msg, gpointer data)
{
	GmpvApplication *app = data;
	const char prefix[] = "gmpv-action";

	if(strncmp(msg, prefix, sizeof(prefix)-1) == 0)
	{
		g_queue_push_head(	app->action_queue,
					g_strdup(msg+sizeof(prefix)) );
		g_idle_add(process_action, app);
	}
	else
	{
		g_warning("Invalid client message received: %s", msg);
	}
}

static void video_reconfig_handler(GmpvMpv *mpv, gpointer data)
{
	GmpvApplication *app = data;
	const GmpvMpvState *state = gmpv_mpv_get_state(mpv);
	gdouble autofit_ratio = gmpv_mpv_get_autofit_ratio(app->mpv);

	if(state->new_file && autofit_ratio > 0)
	{
		resize_window_to_fit(app, autofit_ratio);
	}
}

static void shutdown_handler(GmpvMpv *mpv, gpointer data)
{
	gmpv_application_quit(GMPV_APPLICATION(data));
}

static void mpv_error_handler(GmpvMpv *mpv, const gchar *err, gpointer data)
{
	GmpvApplication *app = data;

	gmpv_main_window_reset(app->gui);
	show_message_dialog(app, GTK_MESSAGE_ERROR, NULL, err, _("Error"));
}

static gboolean shutdown_signal_handler(gpointer data)
{
	g_info("Shutdown signal received. Terminating...");
	gmpv_application_quit(data);

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
			gmpv_mpv_load_list(	app->mpv,
						(const gchar **)uri_list,
						append,
						TRUE );
		}
		else
		{
			gmpv_mpv_load(	app->mpv,
					(const gchar *)raw_data,
					append,
					TRUE );
		}
	}

	g_strfreev(uri_list);
	g_free(type);
}

static gboolean window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GtkApplication *app = data;
	GdkEventWindowState *state = (GdkEventWindowState *)event;

	if(state->changed_mask&GDK_WINDOW_STATE_FULLSCREEN)
	{
		gboolean fullscreen;
		GAction *action;

		fullscreen =	state->new_window_state&
				GDK_WINDOW_STATE_FULLSCREEN;

		action = g_action_map_lookup_action(	G_ACTION_MAP(app),
							"toggle-playlist" );
		g_object_set(action, "enabled", !fullscreen, NULL);

		action = g_action_map_lookup_action(	G_ACTION_MAP(app),
							"set-video-size" );
		g_object_set(action, "enabled", !fullscreen, NULL);
	}

	return FALSE;
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

	rc = gmpv_mpv_get_property(	app->mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&mpv_pos );

	gmpv_mpv_set_property_flag(app->mpv, "pause", FALSE);

	if(pos != mpv_pos)
	{
		if(rc >= 0)
		{
			gmpv_mpv_set_property(	app->mpv,
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

static gboolean load_files(	GmpvApplication *app,
				const gchar **files,
				gboolean append )
{
	if(files)
	{
		gmpv_mpv_load_list(app->mpv, files, append, TRUE);

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

	gmpv_inputctl_connect_signals(app);
	gmpv_playbackctl_connect_signals(app);

	g_unix_signal_add(SIGHUP, shutdown_signal_handler, app);
	g_unix_signal_add(SIGINT, shutdown_signal_handler, app);
	g_unix_signal_add(SIGTERM, shutdown_signal_handler, app);

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
				"window-state-event",
				G_CALLBACK(window_state_handler),
				app );
	g_signal_connect(	app->gui,
				"grab-notify",
				G_CALLBACK(grab_handler),
				app );
	g_signal_connect(	app->gui,
				"delete-event",
				G_CALLBACK(delete_handler),
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
				"load",
				G_CALLBACK(load_handler),
				app );
	g_signal_connect(	app->mpv,
				"idle",
				G_CALLBACK(idle_handler),
				app );
	g_signal_connect(	app->mpv,
				"message",
				G_CALLBACK(message_handler),
				app );
	g_signal_connect(	app->mpv,
				"video-reconfig",
				G_CALLBACK(video_reconfig_handler),
				app );
	g_signal_connect(	app->mpv,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				app );
	g_signal_connect(	app->mpv,
				"mpv-prop-change",
				G_CALLBACK(mpv_prop_change_handler),
				app );
	g_signal_connect(	app->mpv,
				"mpv-error",
				G_CALLBACK(mpv_error_handler),
				app );
}

static void gmpv_application_class_init(GmpvApplicationClass *klass)
{
}

static void gmpv_application_init(GmpvApplication *app)
{
	app->no_existing_session = FALSE;
	app->action_queue = g_queue_new();

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

void gmpv_application_quit(GmpvApplication *app)
{
	const gchar *cmd[] = {"quit", NULL};
	GmpvMpv *mpv = gmpv_application_get_mpv(app);
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);

	if(gmpv_mpv_get_mpv_handle(mpv))
	{
		GmpvVideoArea *vid_area = gmpv_main_window_get_video_area(wnd);
		GtkGLArea *gl_area = gmpv_video_area_get_gl_area(vid_area);

		if(gtk_widget_get_realized(GTK_WIDGET(gl_area)))
		{
			/* Needed by gmpv_mpv_quit() to uninitialize
			 * opengl-cb
			 */
			gtk_gl_area_make_current(GTK_GL_AREA(gl_area));
		}

		gmpv_mpv_command(mpv, cmd);
		gmpv_mpv_quit(mpv);
	}

	if(!gmpv_main_window_get_fullscreen(wnd))
	{
		gmpv_main_window_save_state(wnd);
	}

	g_application_quit(G_APPLICATION(app));
}

