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

#include <epoxy/gl.h>

#include "gmpv_model.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_wrapper.h"

enum
{
	PROP_INVALID,
	PROP_MPV,
	PROP_READY,
	PROP_AID,
	PROP_VID,
	PROP_SID,
	PROP_CHAPTERS,
	PROP_CORE_IDLE,
	PROP_IDLE_ACTIVE,
	PROP_FULLSCREEN,
	PROP_PAUSE,
	PROP_LOOP,
	PROP_DURATION,
	PROP_MEDIA_TITLE,
	PROP_METADATA,
	PROP_PLAYLIST,
	PROP_PLAYLIST_COUNT,
	PROP_PLAYLIST_POS,
	PROP_SPEED,
	PROP_TRACK_LIST,
	PROP_VOLUME,
	N_PROPERTIES
};

struct _GmpvModel
{
	GObject parent;
	GmpvMpv *mpv;
	gboolean ready;
	gboolean update_mpv_properties;
	gchar *aid;
	gchar *vid;
	gchar *sid;
	gint64 chapters;
	gboolean core_idle;
	gboolean idle_active;
	gboolean fullscreen;
	gboolean pause;
	gchar *loop;
	gdouble duration;
	gchar *media_title;
	GPtrArray *metadata;
	GPtrArray *playlist;
	gint64 playlist_count;
	gint64 playlist_pos;
	gdouble speed;
	GPtrArray *track_list;
	gdouble volume;
};

struct _GmpvModelClass
{
	GObjectClass parent_class;
};

static void constructed(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void set_mpv_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void g_value_set_by_type(GValue *gvalue, GType type, gpointer value);
static GParamSpec *g_param_spec_by_type(	const gchar *name,
						const gchar *nick,
						const gchar *blurb,
						GType type,
						GParamFlags flags );
static gboolean emit_frame_ready(gpointer data);
static void opengl_cb_update_callback(gpointer opengl_cb_ctx);
static void autofit_handler(GmpvMpv *mpv, gdouble ratio, gpointer data);
static void mpv_init_handler(GmpvMpv *mpv, gpointer data);
static void mpv_playback_restart_handler(GmpvMpv *mpv, gpointer data);
static void mpv_prop_change_handler(	GmpvMpv *mpv,
					const gchar *name,
					gpointer value,
					gpointer data );
static void error_handler(GmpvMpv *mpv, const gchar* message, gpointer data);
static void message_handler(GmpvMpv *mpv, const gchar* message, gpointer data);
static void shutdown_handler(GmpvMpv *mpv, gpointer data);

G_DEFINE_TYPE(GmpvModel, gmpv_model, G_TYPE_OBJECT)

static void constructed(GObject *object)
{
	GmpvModel *model = GMPV_MODEL(object);

	g_signal_connect(	model->mpv,
				"autofit",
				G_CALLBACK(autofit_handler),
				model );
	g_signal_connect(	model->mpv,
				"mpv-init",
				G_CALLBACK(mpv_init_handler),
				model );
	g_signal_connect(	model->mpv,
				"mpv-playback-restart",
				G_CALLBACK(mpv_playback_restart_handler),
				model );
	g_signal_connect(	model->mpv,
				"mpv-prop-change",
				G_CALLBACK(mpv_prop_change_handler),
				model );
	g_signal_connect(	model->mpv,
				"error",
				G_CALLBACK(error_handler),
				model );
	g_signal_connect(	model->mpv,
				"message",
				G_CALLBACK(message_handler),
				model );
	g_signal_connect(	model->mpv,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				model );
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvModel *self = GMPV_MODEL(object);

	switch(property_id)
	{
		case PROP_MPV:
		self->mpv = g_value_get_pointer(value);
		break;

		case PROP_READY:
		self->ready = g_value_get_boolean(value);
		break;

		case PROP_AID:
		g_free(self->aid);
		self->aid = g_value_dup_string(value);
		break;

		case PROP_VID:
		g_free(self->vid);
		self->vid = g_value_dup_string(value);
		break;

		case PROP_SID:
		g_free(self->sid);
		self->sid = g_value_dup_string(value);
		break;

		case PROP_CHAPTERS:
		self->chapters = g_value_get_int64(value);
		break;

		case PROP_CORE_IDLE:
		self->core_idle = g_value_get_boolean(value);
		break;

		case PROP_IDLE_ACTIVE:
		self->idle_active = g_value_get_boolean(value);
		break;

		case PROP_FULLSCREEN:
		self->fullscreen = g_value_get_boolean(value);
		break;

		case PROP_PAUSE:
		self->pause = g_value_get_boolean(value);
		break;

		case PROP_LOOP:
		self->loop = g_value_dup_string(value);
		break;

		case PROP_DURATION:
		self->duration = g_value_get_double(value);
		break;

		case PROP_MEDIA_TITLE:
		g_free(self->media_title);
		self->media_title = g_value_dup_string(value);
		break;

		case PROP_METADATA:
		self->metadata = gmpv_mpv_get_metadata(self->mpv);
		break;

		case PROP_PLAYLIST:
		self->playlist = gmpv_mpv_get_playlist(self->mpv);
		break;

		case PROP_PLAYLIST_COUNT:
		self->playlist_count = g_value_get_int64(value);
		break;

		case PROP_PLAYLIST_POS:
		self->playlist_pos = g_value_get_int64(value);
		break;

		case PROP_SPEED:
		self->speed = g_value_get_double(value);
		break;

		case PROP_TRACK_LIST:
		self->track_list = gmpv_mpv_get_track_list(self->mpv);
		break;

		case PROP_VOLUME:
		self->volume = g_value_get_double(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	/* Do not propagate changes from mpv back to itself */
	if(self->update_mpv_properties)
	{
		set_mpv_property(object, property_id, value, pspec);
	}
}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvModel *self = GMPV_MODEL(object);

	switch(property_id)
	{
		case PROP_MPV:
		g_value_set_pointer(value, self->mpv);
		break;

		case PROP_READY:
		g_value_set_boolean(value, self->ready);
		break;

		case PROP_AID:
		g_value_set_string(value, self->aid);
		break;

		case PROP_VID:
		g_value_set_string(value, self->vid);
		break;

		case PROP_SID:
		g_value_set_string(value, self->sid);
		break;

		case PROP_CHAPTERS:
		g_value_set_int64(value, self->chapters);
		break;

		case PROP_CORE_IDLE:
		g_value_set_boolean(value, self->core_idle);
		break;

		case PROP_IDLE_ACTIVE:
		g_value_set_boolean(value, self->idle_active);
		break;

		case PROP_FULLSCREEN:
		g_value_set_boolean(value, self->fullscreen);
		break;

		case PROP_PAUSE:
		g_value_set_boolean(value, self->pause);
		break;

		case PROP_LOOP:
		g_value_set_string(value, self->loop);
		break;

		case PROP_DURATION:
		g_value_set_double(value, self->duration);
		break;

		case PROP_MEDIA_TITLE:
		g_value_set_string(value, self->media_title);
		break;

		case PROP_METADATA:
		g_value_set_pointer(value, self->metadata);
		break;

		case PROP_PLAYLIST:
		g_value_set_pointer(value, self->playlist);
		break;

		case PROP_PLAYLIST_COUNT:
		g_value_set_int64(value, self->playlist_count);
		break;

		case PROP_PLAYLIST_POS:
		g_value_set_int64(value, self->idle_active?0:self->playlist_pos);
		break;

		case PROP_SPEED:
		g_value_set_double(value, self->speed);
		break;

		case PROP_TRACK_LIST:
		g_value_set_pointer(value, self->track_list);
		break;

		case PROP_VOLUME:
		g_value_set_double(value, self->volume);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void set_mpv_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvModel *self = GMPV_MODEL(object);

	switch(property_id)
	{
		case PROP_AID:
		gmpv_mpv_set_property(	self->mpv,
					"aid",
					MPV_FORMAT_STRING,
					&self->aid );
		break;

		case PROP_VID:
		gmpv_mpv_set_property(	self->mpv,
					"vid",
					MPV_FORMAT_STRING,
					&self->vid );
		break;

		case PROP_SID:
		gmpv_mpv_set_property(	self->mpv,
					"sid",
					MPV_FORMAT_STRING,
					&self->sid );
		break;

		case PROP_FULLSCREEN:
		gmpv_mpv_set_property(	self->mpv,
					"fullscreen",
					MPV_FORMAT_FLAG,
					&self->fullscreen );
		break;

		case PROP_PAUSE:
		gmpv_mpv_set_property(	self->mpv,
					"pause",
					MPV_FORMAT_FLAG,
					&self->pause );
		break;

		case PROP_LOOP:
		gmpv_mpv_set_property(	self->mpv,
					"loop",
					MPV_FORMAT_STRING,
					&self->loop );
		break;

		case PROP_MEDIA_TITLE:
		gmpv_mpv_set_property(	self->mpv,
					"media-title",
					MPV_FORMAT_INT64,
					&self->media_title );
		break;

		case PROP_PLAYLIST_POS:
		gmpv_mpv_set_property(	self->mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&self->playlist_pos );
		break;

		case PROP_SPEED:
		gmpv_mpv_set_property(	self->mpv,
					"speed",
					MPV_FORMAT_DOUBLE,
					&self->speed );
		break;

		case PROP_VOLUME:
		gmpv_mpv_set_property(	self->mpv,
					"volume",
					MPV_FORMAT_DOUBLE,
					&self->volume );
		break;
	}
}

static void g_value_set_by_type(GValue *gvalue, GType type, gpointer value)
{
	g_value_unset(gvalue);
	g_value_init(gvalue, type);

	switch(type)
	{
		case G_TYPE_STRING:
		g_value_set_string(gvalue, *((const gchar **)value));
		break;

		case G_TYPE_BOOLEAN:
		g_value_set_boolean(gvalue, *((gboolean *)value));
		break;

		case G_TYPE_INT64:
		g_value_set_int64(gvalue, *((gint64 *)value));
		break;

		case G_TYPE_DOUBLE:
		g_value_set_double(gvalue, *((gdouble *)value));
		break;

		case G_TYPE_POINTER:
		g_value_set_pointer(gvalue, *((gpointer *)value));
		break;

		default:
		g_assert_not_reached();
		break;
	}
}

static GParamSpec *g_param_spec_by_type(	const gchar *name,
						const gchar *nick,
						const gchar *blurb,
						GType type,
						GParamFlags flags )
{
	GParamSpec *result = NULL;

	switch(type)
	{
		case G_TYPE_STRING:
		result = g_param_spec_string(name, nick, blurb, NULL, flags);
		break;

		case G_TYPE_BOOLEAN:
		result = g_param_spec_boolean(name, nick, blurb, FALSE, flags);
		break;

		case G_TYPE_INT64:
		result = g_param_spec_int64(	name,
						nick,
						blurb,
						G_MININT64,
						G_MAXINT64,
						0,
						flags );
		break;

		case G_TYPE_DOUBLE:
		result = g_param_spec_double(	name,
						nick,
						blurb,
						-G_MAXDOUBLE,
						G_MAXDOUBLE,
						0.0,
						flags );
		break;

		case G_TYPE_POINTER:
		result = g_param_spec_pointer(name, nick, blurb, flags);
		break;

		default:
		g_assert_not_reached();
		break;
	}

	return result;
}

static gboolean emit_frame_ready(gpointer data)
{
	g_signal_emit_by_name(data, "frame-ready");

	return FALSE;
}

static void opengl_cb_update_callback(gpointer data)
{
	g_idle_add_full(	G_PRIORITY_HIGH,
				emit_frame_ready,
				data,
				NULL );
}

static void autofit_handler(GmpvMpv *mpv, gdouble ratio, gpointer data)
{
	g_signal_emit_by_name(data, "autofit", ratio);
}

static void mpv_init_handler(GmpvMpv *mpv, gpointer data)
{
	GMPV_MODEL(data)->ready = TRUE;
	g_object_notify(data, "ready");
}

static void mpv_playback_restart_handler(GmpvMpv *mpv, gpointer data)
{
	g_signal_emit_by_name(data, "playback-restart");
}

static void mpv_prop_change_handler(	GmpvMpv *mpv,
					const gchar *name,
					gpointer value,
					gpointer data )
{
	GObjectClass *klass;
	GParamSpec *pspec;
	GValue gvalue = G_VALUE_INIT;

	klass = G_TYPE_INSTANCE_GET_CLASS(data, GMPV_TYPE_MODEL, GObjectClass);
	pspec = g_object_class_find_property(klass, name);

	if(pspec && value)
	{
		GMPV_MODEL(data)->update_mpv_properties = FALSE;

		g_value_set_by_type(&gvalue, pspec->value_type, value);
		g_object_set_property(data, name, &gvalue);

		GMPV_MODEL(data)->update_mpv_properties = TRUE;
	}

}

static void error_handler(GmpvMpv *mpv, const gchar *message, gpointer data)
{
	g_signal_emit_by_name(data, "error", message);
}

static void message_handler(GmpvMpv *mpv, const gchar* message, gpointer data)
{
	g_signal_emit_by_name(data, "message", message);
}

static void shutdown_handler(GmpvMpv *mpv, gpointer data)
{
	g_signal_emit_by_name(data, "shutdown");
}

static void gmpv_model_class_init(GmpvModelClass *klass)
{
	/* The "no" value of aid, vid, and sid cannot be represented with an
	 * int64, so we need to observe them as string to receive notifications
	 * for all possible values.
	 */
	const struct
	{
		const gchar *name;
		guint id;
		GType type;
	}
	mpv_props[] = {	{"aid", PROP_AID, G_TYPE_STRING},
			{"vid", PROP_VID, G_TYPE_STRING},
			{"sid", PROP_SID, G_TYPE_STRING},
			{"chapters", PROP_CHAPTERS, G_TYPE_INT64},
			{"core-idle", PROP_CORE_IDLE, G_TYPE_BOOLEAN},
			{"idle-active", PROP_IDLE_ACTIVE, G_TYPE_BOOLEAN},
			{"fullscreen", PROP_FULLSCREEN, G_TYPE_BOOLEAN},
			{"pause", PROP_PAUSE, G_TYPE_BOOLEAN},
			{"loop", PROP_LOOP, G_TYPE_STRING},
			{"duration", PROP_DURATION, G_TYPE_DOUBLE},
			{"media-title", PROP_MEDIA_TITLE, G_TYPE_STRING},
			{"metadata", PROP_METADATA, G_TYPE_POINTER},
			{"playlist", PROP_PLAYLIST, G_TYPE_POINTER},
			{"playlist-count", PROP_PLAYLIST_COUNT, G_TYPE_INT64},
			{"playlist-pos", PROP_PLAYLIST_POS, G_TYPE_INT64},
			{"speed", PROP_SPEED, G_TYPE_DOUBLE},
			{"track-list", PROP_TRACK_LIST, G_TYPE_POINTER},
			{"volume", PROP_VOLUME, G_TYPE_DOUBLE},
			{NULL, PROP_INVALID, 0} };

	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->constructed = constructed;
	obj_class->set_property = set_property;
	obj_class->get_property = get_property;

	pspec = g_param_spec_pointer
		(	"mpv",
			"mpv",
			"GmpvMpv instance to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_MPV, pspec);

	pspec = g_param_spec_boolean
		(	"ready",
			"Ready",
			"Whether mpv is ready to receive commands",
			FALSE,
			G_PARAM_READABLE );
	g_object_class_install_property(obj_class, PROP_READY, pspec);

	for(int i = 0; mpv_props[i].name; i++)
	{
		pspec = g_param_spec_by_type(	mpv_props[i].name,
						mpv_props[i].name,
						mpv_props[i].name,
						mpv_props[i].type,
						G_PARAM_READWRITE );
		g_object_class_install_property
			(obj_class, mpv_props[i].id, pspec);
	}

	g_signal_new(	"playback-restart",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"frame-ready",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"autofit",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__DOUBLE,
			G_TYPE_NONE,
			1,
			G_TYPE_DOUBLE );
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
	g_signal_new(	"error",
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

static void gmpv_model_init(GmpvModel *model)
{
	model->mpv = NULL;
	model->ready = FALSE;
	model->update_mpv_properties = TRUE;
	model->aid = NULL;
	model->vid = NULL;
	model->sid = NULL;
	model->chapters = 0;
	model->core_idle = FALSE;
	model->idle_active = FALSE;
	model->fullscreen = FALSE;
	model->pause = TRUE;
	model->loop = NULL;
	model->duration = 0.0;
	model->media_title = NULL;
	model->metadata = NULL;
	model->playlist_count = 0;
	model->playlist_pos = 0;
	model->speed = 1.0;
	model->track_list = NULL;
	model->volume = 1.0;
}

GmpvModel *gmpv_model_new(GmpvMpv *mpv)
{
	return GMPV_MODEL(g_object_new(	gmpv_model_get_type(),
					"mpv", mpv,
					NULL ));
}

void gmpv_model_initialize(GmpvModel *model)
{
	gmpv_mpv_initialize(model->mpv);

	gmpv_mpv_set_opengl_cb_callback
		(model->mpv, opengl_cb_update_callback, model);
}

void gmpv_model_reset(GmpvModel *model)
{
	GMPV_MODEL(model)->ready = FALSE;
	g_object_notify(G_OBJECT(model), "ready");

	gmpv_mpv_reset(model->mpv);
}

void gmpv_model_quit(GmpvModel *model)
{
	GMPV_MODEL(model)->ready = FALSE;
	g_object_notify(G_OBJECT(model), "ready");

	gmpv_mpv_quit(model->mpv);
}

void gmpv_model_mouse(GmpvModel *model, gint x, gint y)
{
	gchar *x_str = g_strdup_printf("%d", x);
	gchar *y_str = g_strdup_printf("%d", y);
	const gchar *cmd[] = {"mouse", x_str, y_str, NULL};

	g_debug("Set mouse location to (%s, %s)", x_str, y_str);
	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_key_down(GmpvModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keydown", keystr, NULL};

	g_debug("Sent '%s' key down to mpv", keystr);
	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_key_up(GmpvModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keyup", keystr, NULL};

	g_debug("Sent '%s' key up to mpv", keystr);
	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_key_press(GmpvModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keypress", keystr, NULL};

	g_debug("Sent '%s' key press to mpv", keystr);
	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_reset_keys(GmpvModel *model)
{
	g_debug("Sent global key up to mpv");
	gmpv_mpv_command_string(model->mpv, "keyup");
}

void gmpv_model_play(GmpvModel *model)
{
	gmpv_mpv_set_property_flag(model->mpv, "pause", FALSE);
}

void gmpv_model_pause(GmpvModel *model)
{
	gmpv_mpv_set_property_flag(model->mpv, "pause", TRUE);
}

void gmpv_model_stop(GmpvModel *model)
{
	const gchar *cmd[] = {"stop", NULL};

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_forward(GmpvModel *model)
{
	const gchar *cmd[] = {"seek", "10", NULL};

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_rewind(GmpvModel *model)
{
	const gchar *cmd[] = {"seek", "-10", NULL};

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_next_chapter(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", NULL};

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_previous_chapter(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", "down", NULL};

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_next_playlist_entry(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-next", "weak", NULL};

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_previous_playlist_entry(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-prev", "weak", NULL};

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_seek(GmpvModel *model, gdouble value)
{
	gmpv_mpv_set_property(model->mpv, "time-pos", MPV_FORMAT_DOUBLE, &value);
}

void gmpv_model_seek_offset(GmpvModel *model, gdouble offset)
{
	const gchar *cmd[] = {"seek", NULL, NULL};
	gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

	g_ascii_dtostr(buf, G_ASCII_DTOSTR_BUF_SIZE, offset);
	cmd[1] = buf;

	gmpv_mpv_command(model->mpv, cmd);
}

void gmpv_model_load_audio_track(GmpvModel *model, const gchar *filename)
{
	gmpv_mpv_load_track(model->mpv, filename, TRACK_TYPE_AUDIO);
}

void gmpv_model_load_subtitle_track(GmpvModel *model, const gchar *filename)
{
	gmpv_mpv_load_track(model->mpv, filename, TRACK_TYPE_SUBTITLE);
}

gdouble gmpv_model_get_time_position(GmpvModel *model)
{
	gdouble time_pos = 0.0;

	if(!model->idle_active)
	{
		gmpv_mpv_get_property(	model->mpv,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&time_pos );
	}

	/* time-pos may become negative during seeks */
	return MAX(0, time_pos);
}

void gmpv_model_set_playlist_position(GmpvModel *model, gint64 position)
{
	if(position != model->playlist_pos)
	{
		gmpv_mpv_set_property(	model->mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&position );
	}
}

void gmpv_model_remove_playlist_entry(GmpvModel *model, gint64 position)
{
	const gchar *cmd[] = {"playlist_remove", NULL, NULL};
	gchar *index_str = g_strdup_printf("%" G_GINT64_FORMAT, position);

	cmd[1] = index_str;

	gmpv_mpv_command(model->mpv, cmd);

	g_free(index_str);
}

void gmpv_model_move_playlist_entry(GmpvModel *model, gint64 src, gint64 dst)
{
	const gchar *cmd[] =	{"playlist_move", NULL, NULL, NULL};
	gchar *src_str =	g_strdup_printf
				("%" G_GINT64_FORMAT, (src > dst)?--src:src);
	gchar *dst_str =	g_strdup_printf
				("%" G_GINT64_FORMAT, dst);

	cmd[1] = src_str;
	cmd[2] = dst_str;

	gmpv_mpv_command(model->mpv, cmd);

	g_free(src_str);
	g_free(dst_str);
}

void gmpv_model_load_file(GmpvModel *model, const gchar *uri, gboolean append)
{
	gmpv_mpv_load(model->mpv, uri, append);

	/* Start playing when replacing the playlist, ie. not appending, or
	 * adding the first file to the playlist.
	 */
	if(!append || model->playlist->len == 1)
	{
		gmpv_model_play(model);
	}
}

gboolean gmpv_model_get_use_opengl_cb(GmpvModel *model)
{
	return gmpv_mpv_get_use_opengl_cb(model->mpv);
}

void gmpv_model_initialize_gl(GmpvModel *model)
{
	gmpv_mpv_init_gl(model->mpv);
}

void gmpv_model_render_frame(GmpvModel *model, gint width, gint height)
{
	mpv_opengl_cb_context *opengl_ctx;

	opengl_ctx = gmpv_mpv_get_opengl_cb_context(model->mpv);

	if(opengl_ctx)
	{
		int fbo = -1;

		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
		mpv_opengl_cb_draw(opengl_ctx, fbo, width, (-1)*height);
	}
}

void gmpv_model_get_video_geometry(	GmpvModel *model,
					gint64 *width,
					gint64 *height )
{
	gmpv_mpv_get_property(model->mpv, "dwidth", MPV_FORMAT_INT64, width);
	gmpv_mpv_get_property(model->mpv, "dheight", MPV_FORMAT_INT64, height);
}

gchar *gmpv_model_get_current_path(GmpvModel *model)
{
	gchar *path = gmpv_mpv_get_property_string(model->mpv, "path");
	gchar *buf = g_strdup(path);

	mpv_free(path);

	return buf;
}
