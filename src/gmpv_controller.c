/*
 * Copyright (c) 2017-2019 gnome-mpv
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

#include <glib/gi18n.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glib-unix.h>
#include <locale.h>

#include "gmpv_controller_private.h"
#include "gmpv_controller.h"
#include "gmpv_controller_actions.h"
#include "gmpv_controller_input.h"
#include "gmpv_def.h"

static void constructed(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void dispose(GObject *object);
static gboolean loop_to_boolean(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static gboolean boolean_to_loop(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static void mpris_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data );
static void media_keys_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data );
static void view_ready_handler(GmpvView *view, gpointer data);
static void render_handler(GmpvView *view, gpointer data);
static void preferences_updated_handler(GmpvView *view, gpointer data);
static void audio_track_load_handler(	GmpvView *view,
					const gchar *uri,
					gpointer data );
static void subtitle_track_load_handler(	GmpvView *view,
						const gchar *uri,
						gpointer data );
static void file_open_handler(	GmpvView *view,
				const gchar **uri_list,
				gboolean append,
				gpointer data );
static void grab_handler(GmpvView *view, gboolean was_grabbed, gpointer data);
static gboolean delete_handler(GmpvView *view, GdkEvent *event, gpointer data);
static void playlist_item_activated_handler(	GmpvView *view,
						gint pos,
						gpointer data );
static void playlist_item_deleted_handler(	GmpvView *view,
						gint pos,
						gpointer data );
static void playlist_reordered_handler(	GmpvView *view,
					gint src,
					gint dst,
					gpointer data );
static void set_use_skip_button_for_playlist(	GmpvController *controller,
						gboolean value);
static void connect_signals(GmpvController *controller);
static gboolean update_seek_bar(gpointer data);
static gboolean is_more_than_one(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static void idle_active_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data);
static void playlist_handler(		GObject *object,
					GParamSpec *pspec,
					gpointer data);
static void vid_handler(		GObject *object,
					GParamSpec *pspec,
					gpointer data);
static void window_scale_handler(		GObject *object,
						GParamSpec *pspec,
						gpointer data);
static void model_ready_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data );
static void frame_ready_handler(GmpvModel *model, gpointer data);
static void metadata_update_handler(GmpvModel *model, gint64 pos, gpointer data);
static void window_resize_handler(	GmpvModel *model,
					gint64 width,
					gint64 height,
					gpointer data );
static void window_move_handler(	GmpvModel *model,
					gboolean flip_x,
					gboolean flip_y,
					GValue *x,
					GValue *y,
					gpointer data );
static void message_handler(GmpvMpv *mpv, const gchar *message, gpointer data);
static void error_handler(GmpvMpv *mpv, const gchar *message, gpointer data);
static void shutdown_handler(GmpvMpv *mpv, gpointer data);
static gboolean update_window_scale(gpointer data);
static void video_area_resize_handler(	GmpvView *view,
					gint width,
					gint height,
					gpointer data );
static void fullscreen_handler(		GObject *object,
					GParamSpec *pspec,
					gpointer data );
static void play_button_handler(GtkButton *button, gpointer data);
static void stop_button_handler(GtkButton *button, gpointer data);
static void forward_button_handler(GtkButton *button, gpointer data);
static void rewind_button_handler(GtkButton *button, gpointer data);
static void next_button_handler(GtkButton *button, gpointer data);
static void previous_button_handler(GtkButton *button, gpointer data);
static void fullscreen_button_handler(GtkButton *button, gpointer data);
static void seek_handler(GtkButton *button, gdouble value, gpointer data);
static void gmpv_controller_class_init(GmpvControllerClass *klass);
static void gmpv_controller_init(GmpvController *controller);

G_DEFINE_TYPE(GmpvController, gmpv_controller, G_TYPE_OBJECT)

static void constructed(GObject *object)
{
	GmpvController *controller;
	GmpvMainWindow *window;
	gboolean always_floating;
	gint64 wid;

	controller = GMPV_CONTROLLER(object);
	always_floating =	g_settings_get_boolean
				(	controller->settings,
					"always-use-floating-controls" );

	controller->view = gmpv_view_new(controller->app, always_floating);
	window = GMPV_MAIN_WINDOW(controller->view);
	wid = gmpv_video_area_get_xid(gmpv_main_window_get_video_area(window));
	controller->model = gmpv_model_new(wid);

	connect_signals(controller);
	gmpv_controller_action_register_actions(controller);
	gmpv_controller_input_connect_signals(controller);

	gtk_widget_show_all(GTK_WIDGET(window));

	g_signal_connect(	controller->settings,
				"changed::mpris-enable",
				G_CALLBACK(mpris_enable_handler),
				controller );
	g_signal_connect(	controller->settings,
				"changed::media-keys-enable",
				G_CALLBACK(media_keys_enable_handler),
				controller );

	if(g_settings_get_boolean(controller->settings, "mpris-enable"))
	{
		controller->mpris = gmpv_mpris_new(controller);
	}

	if(g_settings_get_boolean(controller->settings, "media-keys-enable"))
	{
		controller->media_keys = gmpv_media_keys_new(controller);
	}

	G_OBJECT_CLASS(gmpv_controller_parent_class)->constructed(object);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvController *self = GMPV_CONTROLLER(object);

	switch(property_id)
	{
		case PROP_APP:
		self->app = g_value_get_pointer(value);
		break;

		case PROP_READY:
		self->ready = g_value_get_boolean(value);
		break;

		case PROP_IDLE:
		self->idle = g_value_get_boolean(value);
		break;

		case PROP_USE_SKIP_BUTTONS_FOR_PLAYLIST:
		self->use_skip_buttons_for_playlist = g_value_get_boolean(value);
		set_use_skip_button_for_playlist
			(self, self->use_skip_buttons_for_playlist);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvController *self = GMPV_CONTROLLER(object);

	switch(property_id)
	{
		case PROP_APP:
		g_value_set_pointer(value, self->app);
		break;

		case PROP_READY:
		g_value_set_boolean(value, self->ready);
		break;

		case PROP_IDLE:
		g_value_set_boolean(value, self->idle);
		break;

		case PROP_USE_SKIP_BUTTONS_FOR_PLAYLIST:
		g_value_set_boolean(value, self->use_skip_buttons_for_playlist);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void dispose(GObject *object)
{
	GmpvController *controller = GMPV_CONTROLLER(object);

	g_clear_object(&controller->settings);
	g_clear_object(&controller->mpris);
	g_clear_object(&controller->media_keys);

	g_source_clear(&controller->update_seekbar_id);
	g_source_clear(&controller->resize_timeout_tag);

	if(controller->view)
	{
		gmpv_view_make_gl_context_current(controller->view);
		g_clear_object(&controller->model);
		gtk_widget_destroy(GTK_WIDGET(controller->view));
		controller->view = NULL;
	}

	G_OBJECT_CLASS(gmpv_controller_parent_class)->dispose(object);
}

static gboolean loop_to_boolean(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	const gchar *from = g_value_get_string(from_value);
	gboolean to = g_strcmp0(from, "no") != 0;

	g_value_set_boolean(to_value, to);

	return TRUE;
}

static gboolean boolean_to_loop(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	gboolean from = g_value_get_boolean(from_value);
	const gchar *to = from?"inf":"no";

	g_value_set_static_string(to_value, to);

	return TRUE;
}

static void mpris_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data )
{
	GmpvController *controller = data;

	if(!controller->mpris && g_settings_get_boolean(settings, key))
	{
		controller->mpris = gmpv_mpris_new(controller);
	}
	else if(controller->mpris)
	{
		g_clear_object(&controller->mpris);
	}
}

static void media_keys_enable_handler(	GSettings *settings,
					gchar *key,
					gpointer data )
{
	GmpvController *controller = data;

	if(!controller->media_keys && g_settings_get_boolean(settings, key))
	{
		controller->media_keys = gmpv_media_keys_new(controller);
	}
	else if(controller->media_keys)
	{
		g_clear_object(&controller->media_keys);
	}
}

static void view_ready_handler(GmpvView *view, gpointer data)
{
	GmpvController *controller = GMPV_CONTROLLER(data);
	GmpvApplication *app = controller->app;
	GmpvModel *model = controller->model;
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	const gchar *cli_options = gmpv_application_get_mpv_options(app);
	gchar *pref_options = g_settings_get_string(settings, "mpv-options");
	gchar *options = g_strjoin(" ", pref_options, cli_options, NULL);

	g_object_set(model, "extra-options", options, NULL);
	gmpv_model_initialize(model, options);

	g_free(options);
	g_free(pref_options);
	g_object_unref(settings);
}

static void render_handler(GmpvView *view, gpointer data)
{
	GmpvController *controller = data;
	gint scale = 1;
	gint width = -1;
	gint height = -1;

	scale = gmpv_view_get_scale_factor(controller->view);
	gmpv_view_get_video_area_geometry(controller->view, &width, &height);
	gmpv_model_render_frame(controller->model, scale*width, scale*height);
}

static void preferences_updated_handler(GmpvView *view, gpointer data)
{
	gmpv_model_reset(GMPV_CONTROLLER(data)->model);
}

static void audio_track_load_handler(	GmpvView *view,
					const gchar *uri,
					gpointer data )
{
	gmpv_model_load_audio_track(GMPV_CONTROLLER(data)->model, uri);
}

static void subtitle_track_load_handler(	GmpvView *view,
						const gchar *uri,
						gpointer data )
{
	gmpv_model_load_subtitle_track(GMPV_CONTROLLER(data)->model, uri);
}

static void file_open_handler(	GmpvView *view,
				const gchar **uri_list,
				gboolean append,
				gpointer data )
{
	for(const gchar **iter = uri_list; iter && *iter; iter++)
	{
		gmpv_model_load_file(	GMPV_CONTROLLER(data)->model,
					*iter,
					append || iter != uri_list );
	}
}

static void grab_handler(GmpvView *view, gboolean was_grabbed, gpointer data)
{
	if(!was_grabbed)
	{
		gmpv_model_reset_keys(GMPV_CONTROLLER(data)->model);
	}
}

static gboolean delete_handler(GmpvView *view, GdkEvent *event, gpointer data)
{
	g_signal_emit_by_name(data, "shutdown");

	return TRUE;
}

static void playlist_item_activated_handler(	GmpvView *view,
						gint pos,
						gpointer data )
{
	GmpvController *controller = GMPV_CONTROLLER(data);
	gboolean idle_active = FALSE;

	g_object_get(controller->model, "idle-active", &idle_active, NULL);
	gmpv_model_play(controller->model);

	if(idle_active)
	{
		controller->target_playlist_pos = pos;
	}
	else
	{
		gmpv_model_set_playlist_position(controller->model, pos);
	}
}

static void playlist_item_deleted_handler(	GmpvView *view,
						gint pos,
						gpointer data )
{
	gmpv_model_remove_playlist_entry(GMPV_CONTROLLER(data)->model, pos);
}

static void playlist_reordered_handler(	GmpvView *view,
					gint src,
					gint dst,
					gpointer data )
{
	gmpv_model_move_playlist_entry(GMPV_CONTROLLER(data)->model, src, dst);
}

static void set_use_skip_button_for_playlist(	GmpvController *controller,
						gboolean value )
{
	if(controller->skip_buttons_binding)
	{
		g_binding_unbind(controller->skip_buttons_binding);
	}

	controller->skip_buttons_binding
		= g_object_bind_property_full
			(	controller->model,
				value?"playlist-count":"chapters",
				controller->view,
				"skip-enabled",
				G_BINDING_DEFAULT|G_BINDING_SYNC_CREATE,
				is_more_than_one,
				NULL,
				NULL,
				NULL );
}

static void connect_signals(GmpvController *controller)
{
	g_object_bind_property(	controller->model, "core-idle",
				controller, "idle",
				G_BINDING_DEFAULT );
	g_object_bind_property(	controller->model, "fullscreen",
				controller->view, "fullscreen",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	controller->model, "pause",
				controller->view, "pause",
				G_BINDING_DEFAULT );
	g_object_bind_property(	controller->model, "media-title",
				controller->view, "title",
				G_BINDING_DEFAULT );
	g_object_bind_property(	controller->model, "volume",
				controller->view, "volume",
				G_BINDING_BIDIRECTIONAL );
	g_object_bind_property(	controller->model, "duration",
				controller->view, "duration",
				G_BINDING_DEFAULT );
	g_object_bind_property(	controller->model, "playlist-pos",
				controller->view, "playlist-pos",
				G_BINDING_DEFAULT );
	g_object_bind_property(	controller->model, "track-list",
				controller->view, "track-list",
				G_BINDING_DEFAULT );
	g_object_bind_property(	controller->view, "display-fps",
				controller->model, "display-fps",
				G_BINDING_DEFAULT|G_BINDING_SYNC_CREATE );
	g_object_bind_property_full(	controller->model, "loop-playlist",
					controller->view, "loop",
					G_BINDING_BIDIRECTIONAL,
					loop_to_boolean,
					boolean_to_loop,
					NULL,
					NULL );

	g_signal_connect(	controller->model,
				"notify::ready",
				G_CALLBACK(model_ready_handler),
				controller );
	g_signal_connect(	controller->model,
				"notify::idle-active",
				G_CALLBACK(idle_active_handler),
				controller );
	g_signal_connect(	controller->model,
				"notify::playlist",
				G_CALLBACK(playlist_handler),
				controller );
	g_signal_connect(	controller->model,
				"notify::vid",
				G_CALLBACK(vid_handler),
				controller );
	g_signal_connect(	controller->model,
				"notify::window-scale",
				G_CALLBACK(window_scale_handler),
				controller );
	g_signal_connect(	controller->model,
				"frame-ready",
				G_CALLBACK(frame_ready_handler),
				controller );
	g_signal_connect(	controller->model,
				"metadata-update",
				G_CALLBACK(metadata_update_handler),
				controller );
	g_signal_connect(	controller->model,
				"window-resize",
				G_CALLBACK(window_resize_handler),
				controller );
	g_signal_connect(	controller->model,
				"window-move",
				G_CALLBACK(window_move_handler),
				controller );
	g_signal_connect(	controller->model,
				"message",
				G_CALLBACK(message_handler),
				controller );
	g_signal_connect(	controller->model,
				"error",
				G_CALLBACK(error_handler),
				controller );
	g_signal_connect(	controller->model,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				controller );

	g_signal_connect(	controller->view,
				"video-area-resize",
				G_CALLBACK(video_area_resize_handler),
				controller );
	g_signal_connect(	controller->view,
				"notify::fullscreen",
				G_CALLBACK(fullscreen_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-clicked::play",
				G_CALLBACK(play_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-clicked::stop",
				G_CALLBACK(stop_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-clicked::forward",
				G_CALLBACK(forward_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-clicked::rewind",
				G_CALLBACK(rewind_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-clicked::next",
				G_CALLBACK(next_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-clicked::previous",
				G_CALLBACK(previous_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"button-clicked::fullscreen",
				G_CALLBACK(fullscreen_button_handler),
				controller );
	g_signal_connect(	controller->view,
				"seek",
				G_CALLBACK(seek_handler),
				controller );

	g_signal_connect(	controller->view,
				"ready",
				G_CALLBACK(view_ready_handler),
				controller );
	g_signal_connect(	controller->view,
				"render",
				G_CALLBACK(render_handler),
				controller );
	g_signal_connect(	controller->view,
				"preferences-updated",
				G_CALLBACK(preferences_updated_handler),
				controller );
	g_signal_connect(	controller->view,
				"audio-track-load",
				G_CALLBACK(audio_track_load_handler),
				controller );
	g_signal_connect(	controller->view,
				"subtitle-track-load",
				G_CALLBACK(subtitle_track_load_handler),
				controller );
	g_signal_connect(	controller->view,
				"file-open",
				G_CALLBACK(file_open_handler),
				controller );
	g_signal_connect(	controller->view,
				"grab-notify",
				G_CALLBACK(grab_handler),
				controller );
	g_signal_connect(	controller->view,
				"delete-event",
				G_CALLBACK(delete_handler),
				controller );
	g_signal_connect(	controller->view,
				"playlist-item-activated",
				G_CALLBACK(playlist_item_activated_handler),
				controller );
	g_signal_connect(	controller->view,
				"playlist-item-deleted",
				G_CALLBACK(playlist_item_deleted_handler),
				controller );
	g_signal_connect(	controller->view,
				"playlist-reordered",
				G_CALLBACK(playlist_reordered_handler),
				controller );
}

gboolean update_seek_bar(gpointer data)
{
	GmpvController *controller = data;
	gdouble time_pos = gmpv_model_get_time_position(controller->model);

	gmpv_view_set_time_position(controller->view, time_pos);

	return TRUE;
}

static gboolean is_more_than_one(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	gint64 from = g_value_get_int64(from_value);

	g_value_set_boolean(to_value, from > 1);

	return TRUE;
}

static void idle_active_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	GmpvController *controller = data;
	gboolean idle_active = TRUE;

	g_object_get(object, "idle-active", &idle_active, NULL);

	if(idle_active)
	{
		gmpv_view_reset(GMPV_CONTROLLER(data)->view);
	}
	else if(controller->target_playlist_pos >= 0)
	{
		gmpv_model_set_playlist_position
			(controller->model, controller->target_playlist_pos);
	}
}

static void playlist_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvView *view = GMPV_CONTROLLER(data)->view;
	GPtrArray *playlist = NULL;
	gint64 pos = 0;

	g_object_get(	object,
			"playlist", &playlist,
			"playlist-pos", &pos,
			NULL );

	gmpv_view_update_playlist(view, playlist);
	gmpv_view_set_playlist_pos(view, pos);
}

static void vid_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvController *controller = data;
	GmpvMainWindow *window = gmpv_view_get_main_window(controller->view);
	GActionMap *map = G_ACTION_MAP(window);
	GAction *action = g_action_map_lookup_action(map, "set-video-size");
	gchar *vid_str = NULL;
	gint64 vid = 0;

	g_object_get(object, "vid", &vid_str, NULL);
	vid = g_ascii_strtoll(vid_str, NULL, 10);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(action), vid > 0);

	g_free(vid_str);
}

static void window_scale_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	GmpvController *controller = data;
	gint64 video_width = 1;
	gint64 video_height = 1;
	gint width = 1;
	gint height = 1;
	gdouble window_scale = 1.0;
	gdouble new_window_scale = 1.0;

	gmpv_model_get_video_geometry(	controller->model,
					&video_width,
					&video_height );
	gmpv_view_get_video_area_geometry(controller->view, &width, &height);
	g_object_get(object, "window-scale", &new_window_scale, NULL);

	window_scale = MIN(	width/(gdouble)video_width,
				height/(gdouble)video_height );

	if(ABS(window_scale-new_window_scale) > 0.0001)
	{
		gmpv_controller_autofit(data, new_window_scale);
	}
}

static void model_ready_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	GmpvController *controller = data;
	gboolean ready = FALSE;

	g_object_get(object, "ready", &ready, NULL);

	if(ready)
	{
		gboolean use_opengl_cb;

		use_opengl_cb = gmpv_model_get_use_opengl_cb(controller->model);

		gmpv_view_set_use_opengl_cb(controller->view, use_opengl_cb);

		if(use_opengl_cb)
		{
			gmpv_view_make_gl_context_current(controller->view);
			gmpv_model_initialize_gl(controller->model);
		}
	}

	g_source_clear(&controller->update_seekbar_id);
	controller->update_seekbar_id
		= g_timeout_add(	SEEK_BAR_UPDATE_INTERVAL,
					(GSourceFunc)update_seek_bar,
					controller );

	controller->ready = ready;
	g_object_notify(data, "ready");
}

static void frame_ready_handler(GmpvModel *model, gpointer data)
{
	gmpv_view_queue_render(GMPV_CONTROLLER(data)->view);
}

static void metadata_update_handler(GmpvModel *model, gint64 pos, gpointer data)
{
	GmpvView *view = GMPV_CONTROLLER(data)->view;
	GPtrArray *playlist = NULL;

	g_object_get(G_OBJECT(model), "playlist", &playlist, NULL);
	gmpv_view_update_playlist(view, playlist);
}

static void window_resize_handler(	GmpvModel *model,
					gint64 width,
					gint64 height,
					gpointer data )
{
	GmpvController *controller = data;

	gmpv_view_resize_video_area(controller->view, (gint)width, (gint)height);
}

static void window_move_handler(	GmpvModel *model,
					gboolean flip_x,
					gboolean flip_y,
					GValue *x,
					GValue *y,
					gpointer data )
{
	gmpv_view_move(GMPV_CONTROLLER(data)->view, flip_x, flip_y, x, y);
}

static void message_handler(GmpvMpv *mpv, const gchar *message, gpointer data)
{
	const gsize prefix_length = sizeof(ACTION_PREFIX)-1;

	/* Verify that both the prefix and the scope matches */
	if(message && strncmp(message, ACTION_PREFIX, prefix_length) == 0)
	{
		const gchar *action = message+prefix_length+1;
		GmpvController *controller = data;
		GmpvView *view = controller->view;
		GActionMap *map = NULL;

		if(g_str_has_prefix(action, "win."))
		{
			map = G_ACTION_MAP(gmpv_view_get_main_window(view));
		}
		else if(g_str_has_prefix(action, "app."))
		{
			map = G_ACTION_MAP(controller->app);
		}

		if(map)
		{
			/* Strip scope and activate */
			activate_action_string(map, strchr(action, '.')+1);
		}
		else
		{
			g_warning(	"Received action with missing or "
					"unknown scope %s",
					action );
		}
	}
}

static void error_handler(GmpvMpv *mpv, const gchar *message, gpointer data)
{
	gmpv_view_show_message_dialog(	GMPV_CONTROLLER(data)->view,
					GTK_MESSAGE_ERROR,
					_("Error"),
					NULL,
					message );
}

static void shutdown_handler(GmpvMpv *mpv, gpointer data)
{
	g_signal_emit_by_name(data, "shutdown");
}

static gboolean update_window_scale(gpointer data)
{
	gpointer *params = data;
	GmpvController *controller = GMPV_CONTROLLER(params[0]);
	gint64 video_width = 1;
	gint64 video_height = 1;
	gint width = 1;
	gint height = 1;
	gdouble window_scale = 0;
	gdouble new_window_scale = 0;

	gmpv_model_get_video_geometry(	controller->model,
					&video_width,
					&video_height );
	gmpv_view_get_video_area_geometry(controller->view, &width, &height);

	new_window_scale = MIN(	width/(gdouble)video_width,
				height/(gdouble)video_height );
	g_object_get(controller->model, "window-scale", &window_scale, NULL);

	if(ABS(window_scale-new_window_scale) > 0.0001)
	{
		g_object_set(	controller->model,
				"window-scale",
				new_window_scale,
				NULL );
	}

	// Clear event source ID for the timeout
	*((guint *)params[1]) = 0;
	g_free(params);

	return FALSE;
}

static void video_area_resize_handler(	GmpvView *view,
					gint width,
					gint height,
					gpointer data )
{
	GmpvController *controller = data;
	gpointer *params = g_new(gpointer, 2);

	g_source_clear(&controller->resize_timeout_tag);

	params[0] = data;
	params[1] = &(controller->resize_timeout_tag);

	// Rate-limit the call to update_window_scale(), which will update the
	// window-scale property in the model, to no more than once every 250ms.
	controller->resize_timeout_tag = g_timeout_add(	250,
							update_window_scale,
							params );
}

static void fullscreen_handler(		GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	GmpvView *view = GMPV_CONTROLLER(data)->view;
	GmpvMainWindow *window = gmpv_view_get_main_window(view);
	GActionMap *map = G_ACTION_MAP(window);
	GAction *toggle_playlist = NULL;
	GAction *toggle_controls = NULL;
	gboolean fullscreen = FALSE;

	toggle_playlist = g_action_map_lookup_action(map, "toggle-playlist");
	toggle_controls = g_action_map_lookup_action(map, "toggle-controls");

	g_object_get(view, "fullscreen", &fullscreen, NULL);

	g_simple_action_set_enabled
		(G_SIMPLE_ACTION(toggle_playlist), !fullscreen);
	g_simple_action_set_enabled
		(G_SIMPLE_ACTION(toggle_controls), !fullscreen);
}

static void play_button_handler(GtkButton *button, gpointer data)
{
	GmpvModel *model = GMPV_CONTROLLER(data)->model;
	gboolean pause = TRUE;

	g_object_get(model, "pause", &pause, NULL);

	if(pause)
	{
		gmpv_model_play(model);
	}
	else
	{
		gmpv_model_pause(model);
	}
}

static void stop_button_handler(GtkButton *button, gpointer data)
{
	gmpv_model_stop(GMPV_CONTROLLER(data)->model);
}

static void forward_button_handler(GtkButton *button, gpointer data)
{
	gmpv_model_forward(GMPV_CONTROLLER(data)->model);
}

static void rewind_button_handler(GtkButton *button, gpointer data)
{
	gmpv_model_rewind(GMPV_CONTROLLER(data)->model);
}

static void next_button_handler(GtkButton *button, gpointer data)
{
	GmpvController *controller = data;

	if(controller->use_skip_buttons_for_playlist)
	{
		gmpv_model_next_playlist_entry(GMPV_CONTROLLER(data)->model);
	}
	else
	{
		gmpv_model_next_chapter(GMPV_CONTROLLER(data)->model);
	}
}

static void previous_button_handler(GtkButton *button, gpointer data)
{
	GmpvController *controller = data;

	if(controller->use_skip_buttons_for_playlist)
	{
		gmpv_model_previous_playlist_entry(GMPV_CONTROLLER(data)->model);
	}
	else
	{
		gmpv_model_previous_chapter(GMPV_CONTROLLER(data)->model);
	}
}

static void fullscreen_button_handler(GtkButton *button, gpointer data)
{
	GmpvView *view = GMPV_CONTROLLER(data)->view;
	gboolean fullscreen = FALSE;

	g_object_get(view, "fullscreen", &fullscreen, NULL);
	g_object_set(view, "fullscreen", !fullscreen, NULL);
}

static void seek_handler(GtkButton *button, gdouble value, gpointer data)
{
	gmpv_model_seek(GMPV_CONTROLLER(data)->model, value);
}

static void gmpv_controller_class_init(GmpvControllerClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	obj_class->constructed = constructed;
	obj_class->set_property = set_property;
	obj_class->get_property = get_property;
	obj_class->dispose = dispose;

	pspec = g_param_spec_pointer
		(	"app",
			"App",
			"The GmpvApplication to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_APP, pspec);

	pspec = g_param_spec_boolean
		(	"ready",
			"Ready",
			"Whether mpv is ready to receive commands",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_READY, pspec);

	pspec = g_param_spec_boolean
		(	"idle",
			"Idle",
			"Whether or not the player is idle",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_IDLE, pspec);

	pspec = g_param_spec_boolean
		(	"use-skip-buttons-for-playlist",
			"Use skip buttons for playlist",
			"Whether or not to use the skip buttons to control the playlist",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_USE_SKIP_BUTTONS_FOR_PLAYLIST, pspec);

	g_signal_new(	"shutdown",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
}

static void gmpv_controller_init(GmpvController *controller)
{
	controller->app = NULL;
	controller->model = NULL;
	controller->view = NULL;
	controller->ready = FALSE;
	controller->idle = TRUE;
	controller->target_playlist_pos = -1;
	controller->update_seekbar_id = 0;
	controller->resize_timeout_tag = 0;
	controller->skip_buttons_binding = NULL;
	controller->settings = g_settings_new(CONFIG_ROOT);
	controller->media_keys = NULL;
	controller->mpris = NULL;
}

GmpvController *gmpv_controller_new(GmpvApplication *app)
{
	return GMPV_CONTROLLER(g_object_new(	gmpv_controller_get_type(),
						"app", app,
						NULL ));
}

void gmpv_controller_quit(GmpvController *controller)
{
	gmpv_view_quit(controller->view);
}

void gmpv_controller_autofit(GmpvController *controller, gdouble multiplier)
{
	gchar *vid = NULL;
	gint64 width = -1;
	gint64 height = -1;

	g_object_get(G_OBJECT(controller->model), "vid", &vid, NULL);
	gmpv_model_get_video_geometry(controller->model, &width, &height);

	if(vid && strncmp(vid, "no", 3) != 0 && width >= 0 && width >= 0)
	{
		gint new_width = (gint)(multiplier*(gdouble)width);
		gint new_height = (gint)(multiplier*(gdouble)height);

		g_debug("Resizing window to %dx%d", new_width, new_height);
		gmpv_view_resize_video_area(	controller->view,
						new_width,
						new_height );
	}

	g_free(vid);
}

void gmpv_controller_present(GmpvController *controller)
{
	gmpv_view_present(controller->view);
}

void gmpv_controller_open(	GmpvController *controller,
				const gchar *uri,
				gboolean append )
{
	gmpv_model_load_file(controller->model, uri, append);
}

GmpvView *gmpv_controller_get_view(GmpvController *controller)
{
	return controller->view;
}

GmpvModel *gmpv_controller_get_model(GmpvController *controller)
{
	return controller->model;
}
