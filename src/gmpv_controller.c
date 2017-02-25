/*
 * Copyright (c) 2017 gnome-mpv
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
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glib-unix.h>
#include <locale.h>

#include "gmpv_controller_private.h"
#include "gmpv_controller.h"
#include "gmpv_inputctl.h"
#include "gmpv_track.h"
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
static void delete_handler(GmpvView *view, gpointer data);
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
static void connect_signals(GmpvController *controller);
static gboolean track_str_to_int(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static gboolean int_to_track_str(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static gboolean loop_str_to_boolean(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static gboolean boolean_to_loop_str(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static gboolean is_more_than_one(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data );
static void idle_active_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data);
static void model_ready_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data );
static void frame_ready_handler(GmpvModel *model, gpointer data);
static void autofit_handler(GmpvModel *model, gdouble multiplier, gpointer data);
static void message_handler(GmpvMpv *mpv, const gchar *message, gpointer data);
static void shutdown_handler(GmpvMpv *mpv, gpointer data);
static void post_shutdown_handler(GmpvMpv *mpv, gpointer data);
static void play_button_handler(GtkButton *button, gpointer data);
static void stop_button_handler(GtkButton *button, gpointer data);
static void forward_button_handler(GtkButton *button, gpointer data);
static void rewind_button_handler(GtkButton *button, gpointer data);
static void next_button_handler(GtkButton *button, gpointer data);
static void previous_button_handler(GtkButton *button, gpointer data);
static void seek_handler(GtkButton *button, gdouble value, gpointer data);
static void gmpv_controller_class_init(GmpvControllerClass *klass);
static void gmpv_controller_init(GmpvController *controller);

G_DEFINE_TYPE(GmpvController, gmpv_controller, G_TYPE_OBJECT)

static void constructed(GObject *object)
{
	GmpvController *controller = GMPV_CONTROLLER(object);

	connect_signals(controller);
	gmpv_inputctl_connect_signals(controller);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvController *self = GMPV_CONTROLLER(object);

	switch(property_id)
	{
		case PROP_MODEL:
		self->model = g_value_get_pointer(value);
		break;

		case PROP_VIEW:
		self->view = g_value_get_pointer(value);
		break;

		case PROP_READY:
		self->ready = g_value_get_boolean(value);
		break;

		case PROP_AID:
		self->aid = g_value_get_int(value);
		break;

		case PROP_VID:
		self->vid = g_value_get_int(value);
		break;

		case PROP_SID:
		self->sid = g_value_get_int(value);
		break;

		case PROP_LOOP:
		self->loop = g_value_get_boolean(value);
		break;

		case PROP_IDLE:
		self->idle = g_value_get_boolean(value);
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
		case PROP_MODEL:
		g_value_set_pointer(value, self->model);
		break;

		case PROP_VIEW:
		g_value_set_pointer(value, self->view);
		break;

		case PROP_READY:
		g_value_set_boolean(value, self->ready);
		break;

		case PROP_AID:
		g_value_set_int(value, self->aid);
		break;

		case PROP_VID:
		g_value_set_int(value, self->vid);
		break;

		case PROP_SID:
		g_value_set_int(value, self->sid);
		break;

		case PROP_LOOP:
		g_value_set_boolean(value, self->loop);
		break;

		case PROP_IDLE:
		g_value_set_boolean(value, self->idle);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void view_ready_handler(GmpvView *view, gpointer data)
{
	gmpv_model_initialize(GMPV_CONTROLLER(data)->model);
}

static void render_handler(GmpvView *view, gpointer data)
{
	GmpvController *controller = data;
	gint width = -1;
	gint height = -1;

	gmpv_view_get_video_area_geometry(controller->view, &width, &height);
	gmpv_model_render_frame(controller->model, width, height);

	while(gtk_events_pending())
	{
		gtk_main_iteration();
	}
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

static void delete_handler(GmpvView *view, gpointer data)
{
	gmpv_model_quit(GMPV_CONTROLLER(data)->model);
	g_signal_emit_by_name(data, "shutdown");
}

static void playlist_item_activated_handler(	GmpvView *view,
						gint pos,
						gpointer data )
{
	g_object_set(GMPV_CONTROLLER(data)->model, "playlist-pos", pos, NULL);
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

static void connect_signals(GmpvController *controller)
{
	g_object_bind_property_full(	controller->model, "aid",
					controller, "aid",
					G_BINDING_BIDIRECTIONAL,
					track_str_to_int,
					int_to_track_str,
					NULL,
					NULL );
	g_object_bind_property_full(	controller->model, "vid",
					controller, "vid",
					G_BINDING_BIDIRECTIONAL,
					track_str_to_int,
					int_to_track_str,
					NULL,
					NULL );
	g_object_bind_property_full(	controller->model, "sid",
					controller, "sid",
					G_BINDING_BIDIRECTIONAL,
					track_str_to_int,
					int_to_track_str,
					NULL,
					NULL );
	g_object_bind_property_full(	controller->model, "loop",
					controller, "loop",
					G_BINDING_BIDIRECTIONAL,
					loop_str_to_boolean,
					boolean_to_loop_str,
					NULL,
					NULL );
	g_object_bind_property(	controller->model, "idle-active",
				controller, "idle",
				G_BINDING_DEFAULT );

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
	g_object_bind_property_full(	controller->model, "chapters",
					controller->view, "chapters-enabled",
					G_BINDING_DEFAULT,
					is_more_than_one,
					NULL,
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
				"frame-ready",
				G_CALLBACK(frame_ready_handler),
				controller );
	g_signal_connect(	controller->model,
				"autofit",
				G_CALLBACK(autofit_handler),
				controller );
	g_signal_connect(	controller->model,
				"message",
				G_CALLBACK(message_handler),
				controller );
	g_signal_connect(	controller->model,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				controller );
	g_signal_connect_after(	controller->model,
				"shutdown",
				G_CALLBACK(post_shutdown_handler),
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
				"delete-notify",
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

static gboolean track_str_to_int(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	gchar *endptr = NULL;
	const gchar *from = g_value_get_string(from_value);
	gint to = (gint)g_ascii_strtoll(from, &endptr, 10);

	if(to == 0 && g_strcmp0(from, "no") != 0)
	{
		to = -1;
	}

	g_value_set_int(to_value, to);

	return TRUE;
}

static gboolean int_to_track_str(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	gint from = g_value_get_int(from_value);
	gchar buf[16] = "no";

	g_snprintf(buf, 16, "%d", from);
	g_value_set_string(to_value, buf);

	return TRUE;
}

static gboolean loop_str_to_boolean(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	const gchar *from = g_value_get_string(from_value);

	g_value_set_boolean(to_value, g_strcmp0(from, "no") != 0);

	return TRUE;
}

static gboolean boolean_to_loop_str(	GBinding *binding,
					const GValue *from_value,
					GValue *to_value,
					gpointer data )
{
	gboolean from = g_value_get_boolean(from_value);

	g_value_set_string(to_value, from?"inf":"no");

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
	gboolean idle_active = TRUE;

	g_object_get(object, "idle-active", &idle_active, NULL);

	if(idle_active)
	{
		gmpv_view_reset(GMPV_CONTROLLER(data)->view);
	}
}

static void model_ready_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	GmpvController *controller = data;

	if(gmpv_model_get_use_opengl_cb(controller->model))
	{
		gmpv_view_set_use_opengl_cb(controller->view, TRUE);
		gmpv_view_make_gl_context_current(controller->view);
		gmpv_model_initialize_gl(controller->model);
	}

	controller->ready = TRUE;
	g_object_notify(data, "ready");
}

static void frame_ready_handler(GmpvModel *model, gpointer data)
{
	gmpv_view_queue_render(GMPV_CONTROLLER(data)->view);
}

static void autofit_handler(GmpvModel *model, gdouble multiplier, gpointer data)
{
	GmpvController *controller = data;
	gint64 width = -1;
	gint64 height = -1;

	gmpv_model_get_video_geometry(model, &width, &height);

	if(width > 0 && height > 0)
	{
		gint new_width;
		gint new_height;

		new_width = (gint)(multiplier*(gdouble)width);
		new_height = (gint)(multiplier*(gdouble)height);

		g_debug("Resizing window to %dx%d", new_width, new_height);

		gmpv_view_resize_video_area
			(controller->view, new_width, new_height);
	}

}

static void message_handler(GmpvMpv *mpv, const gchar *message, gpointer data)
{
	g_signal_emit_by_name(data, "message", message);
}

static void shutdown_handler(GmpvMpv *mpv, gpointer data)
{
	gmpv_view_make_gl_context_current(GMPV_CONTROLLER(data)->view);
}

static void post_shutdown_handler(GmpvMpv *mpv, gpointer data)
{
	gmpv_view_quit(GMPV_CONTROLLER(data)->view);
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
	gmpv_model_next_chapter(GMPV_CONTROLLER(data)->model);
}

static void previous_button_handler(GtkButton *button, gpointer data)
{
	gmpv_model_previous_chapter(GMPV_CONTROLLER(data)->model);
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

	pspec = g_param_spec_pointer
		(	"model",
			"Model",
			"The GmpvModel to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_MODEL, pspec);

	pspec = g_param_spec_pointer
		(	"view",
			"View",
			"The GmpvView to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_VIEW, pspec);

	pspec = g_param_spec_int
		(	"aid",
			"Audio track ID",
			"The ID of the current audio track",
			-1,
			G_MAXINT,
			-1,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_AID, pspec);

	pspec = g_param_spec_int
		(	"vid",
			"Video track ID",
			"The ID of the current video track",
			-1,
			G_MAXINT,
			-1,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_VID, pspec);

	pspec = g_param_spec_int
		(	"sid",
			"Subtitle track ID",
			"The ID of the current subtitle track",
			-1,
			G_MAXINT,
			-1,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_SID, pspec);

	pspec = g_param_spec_boolean
		(	"ready",
			"Ready",
			"Whether mpv is ready to receive commands",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_READY, pspec);

	pspec = g_param_spec_boolean
		(	"loop",
			"Loop",
			"Whether or not to loop when the playlist ends",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_LOOP, pspec);

	pspec = g_param_spec_boolean
		(	"idle",
			"Idle",
			"Whether or not the player is idle",
			TRUE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_IDLE, pspec);

	g_signal_new(	"message",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
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
	controller->model = NULL;
	controller->view = NULL;
	controller->aid = 0;
	controller->vid = 0;
	controller->sid = 0;
	controller->ready = FALSE;
	controller->loop = FALSE;
	controller->idle = TRUE;
	controller->action_queue = g_queue_new();
	controller->files = NULL;
	controller->inhibit_cookie = 0;
	controller->target_playlist_pos = -1;
}

GmpvController *gmpv_controller_new(GmpvModel *model, GmpvView *view)
{
	return GMPV_CONTROLLER(g_object_new(	gmpv_controller_get_type(),
						"model", model,
						"view", view,
						NULL ));
}

void gmpv_controller_quit(GmpvController *controller)
{
	gmpv_view_quit(controller->view);
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
