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

#include <epoxy/gl.h>

#include "gmpv_model.h"
#include "gmpv_marshal.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_metadata_cache.h"

enum
{
	PROP_INVALID,
	PROP_AID,
	PROP_VID,
	PROP_SID,
	PROP_CHAPTERS,
	PROP_CORE_IDLE,
	PROP_IDLE_ACTIVE,
	PROP_FULLSCREEN,
	PROP_PAUSE,
	PROP_LOOP_FILE,
	PROP_LOOP_PLAYLIST,
	PROP_DURATION,
	PROP_MEDIA_TITLE,
	PROP_PLAYLIST_COUNT,
	PROP_PLAYLIST_POS,
	PROP_SPEED,
	PROP_VOLUME,
	PROP_WINDOW_SCALE,
	PROP_DISPLAY_FPS,
	N_PROPERTIES
};

struct _GmpvModel
{
	GmpvPlayer parent;
	gchar *extra_options;
	gboolean ready;
	GmpvMetadataCache *cache;
	GPtrArray *metadata;
	GPtrArray *track_list;
	gboolean update_mpv_properties;
	gchar *aid;
	gchar *vid;
	gchar *sid;
	gint64 chapters;
	gboolean core_idle;
	gboolean idle_active;
	gboolean fullscreen;
	gboolean pause;
	gchar *loop_file;
	gchar *loop_playlist;
	gdouble duration;
	gchar *media_title;
	gint64 playlist_count;
	gint64 playlist_pos;
	gdouble speed;
	gdouble volume;
	gdouble window_scale;
	gdouble display_fps;
};

struct _GmpvModelClass
{
	GObjectClass parent_class;
};

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void dispose(GObject *object);
static void finalize(GObject *object);
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
static void render_update_callback(gpointer render_ctx);
static void mpv_prop_change_handler(	GmpvMpv *mpv,
					const gchar *name,
					gpointer value,
					gpointer data );

G_DEFINE_TYPE(GmpvModel, gmpv_model, GMPV_TYPE_PLAYER)

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvModel *self = GMPV_MODEL(object);

	switch(property_id)
	{
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

		if(self->idle_active)
		{
			g_object_notify(object, "playlist-pos");
		}
		break;

		case PROP_FULLSCREEN:
		self->fullscreen = g_value_get_boolean(value);
		break;

		case PROP_PAUSE:
		self->pause = g_value_get_boolean(value);
		break;

		case PROP_LOOP_FILE:
		g_free(self->loop_file);
		self->loop_file = g_value_dup_string(value);
		break;

		case PROP_LOOP_PLAYLIST:
		g_free(self->loop_playlist);
		self->loop_playlist = g_value_dup_string(value);
		break;

		case PROP_DURATION:
		self->duration = g_value_get_double(value);
		break;

		case PROP_MEDIA_TITLE:
		g_free(self->media_title);
		self->media_title = g_value_dup_string(value);
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

		case PROP_VOLUME:
		self->volume = g_value_get_double(value);
		break;

		case PROP_WINDOW_SCALE:
		self->window_scale = g_value_get_double(value);
		break;

		case PROP_DISPLAY_FPS:
		self->display_fps = g_value_get_double(value);
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

		case PROP_LOOP_FILE:
		g_value_set_string(value, self->loop_file);
		break;

		case PROP_LOOP_PLAYLIST:
		g_value_set_string(value, self->loop_playlist);
		break;

		case PROP_DURATION:
		g_value_set_double(value, self->duration);
		break;

		case PROP_MEDIA_TITLE:
		g_value_set_string(value, self->media_title);
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

		case PROP_VOLUME:
		g_value_set_double(value, self->volume);
		break;

		case PROP_WINDOW_SCALE:
		g_value_set_double(value, self->window_scale);
		break;

		case PROP_DISPLAY_FPS:
		g_value_set_double(value, self->display_fps);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void dispose(GObject *object)
{
	GmpvModel *model = GMPV_MODEL(object);
	GmpvMpv *mpv = GMPV_MPV(model);

	if(mpv)
	{
		gmpv_mpv_set_render_update_callback(mpv, NULL, NULL);
		while(g_source_remove_by_user_data(model));
	}

	g_free(model->extra_options);

	G_OBJECT_CLASS(gmpv_model_parent_class)->dispose(object);
}

static void finalize(GObject *object)
{
	GmpvModel *model = GMPV_MODEL(object);

	g_free(model->aid);
	g_free(model->vid);
	g_free(model->sid);
	g_free(model->loop_file);
	g_free(model->loop_playlist);
	g_free(model->media_title);

	G_OBJECT_CLASS(gmpv_model_parent_class)->finalize(object);
}

static void set_mpv_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvModel *self = GMPV_MODEL(object);
	GmpvMpv *mpv = GMPV_MPV(self);

	switch(property_id)
	{
		case PROP_AID:
		gmpv_mpv_set_property(	mpv,
					"aid",
					MPV_FORMAT_STRING,
					&self->aid );
		break;

		case PROP_VID:
		gmpv_mpv_set_property(	mpv,
					"vid",
					MPV_FORMAT_STRING,
					&self->vid );
		break;

		case PROP_SID:
		gmpv_mpv_set_property(	mpv,
					"sid",
					MPV_FORMAT_STRING,
					&self->sid );
		break;

		case PROP_FULLSCREEN:
		gmpv_mpv_set_property(	mpv,
					"fullscreen",
					MPV_FORMAT_FLAG,
					&self->fullscreen );
		break;

		case PROP_PAUSE:
		gmpv_mpv_set_property(	mpv,
					"pause",
					MPV_FORMAT_FLAG,
					&self->pause );
		break;

		case PROP_LOOP_FILE:
		gmpv_mpv_set_property(	mpv,
					"loop-file",
					MPV_FORMAT_STRING,
					&self->loop_file );
		break;

		case PROP_LOOP_PLAYLIST:
		gmpv_mpv_set_property(	mpv,
					"loop-playlist",
					MPV_FORMAT_STRING,
					&self->loop_playlist );
		break;

		case PROP_MEDIA_TITLE:
		gmpv_mpv_set_property(	mpv,
					"media-title",
					MPV_FORMAT_INT64,
					&self->media_title );
		break;

		case PROP_PLAYLIST_POS:
		gmpv_mpv_set_property(	mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&self->playlist_pos );
		break;

		case PROP_SPEED:
		gmpv_mpv_set_property(	mpv,
					"speed",
					MPV_FORMAT_DOUBLE,
					&self->speed );
		break;

		case PROP_VOLUME:
		gmpv_mpv_set_property(	mpv,
					"volume",
					MPV_FORMAT_DOUBLE,
					&self->volume );
		break;

		case PROP_WINDOW_SCALE:
		gmpv_mpv_set_property(	mpv,
					"window-scale",
					MPV_FORMAT_DOUBLE,
					&self->window_scale );
		break;

		case PROP_DISPLAY_FPS:
		gmpv_mpv_set_property(	mpv,
					"display-fps",
					MPV_FORMAT_DOUBLE,
					&self->display_fps );
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
	GmpvModel *model = data;
	guint64 flags = gmpv_mpv_render_context_update(GMPV_MPV(model));

	if(flags&MPV_RENDER_UPDATE_FRAME)
	{
		g_signal_emit_by_name(model, "frame-ready");
	}

	return FALSE;
}

static void render_update_callback(gpointer data)
{
	g_idle_add_full(	G_PRIORITY_HIGH,
				emit_frame_ready,
				data,
				NULL );
}

static void mpv_prop_change_handler(	GmpvMpv *mpv,
					const gchar *name,
					gpointer value,
					gpointer data )
{
	if(	g_strcmp0(name, "playlist") != 0 &&
		g_strcmp0(name, "metadata") != 0 &&
		g_strcmp0(name, "track-list") != 0 )
	{
		GObjectClass *klass;
		GParamSpec *pspec;
		GValue gvalue = G_VALUE_INIT;

		klass =	G_TYPE_INSTANCE_GET_CLASS
			(data, GMPV_TYPE_MODEL, GObjectClass);
		pspec = g_object_class_find_property(klass, name);

		if(pspec && value)
		{
			GMPV_MODEL(data)->update_mpv_properties = FALSE;

			g_value_set_by_type(&gvalue, pspec->value_type, value);
			g_object_set_property(data, name, &gvalue);

			GMPV_MODEL(data)->update_mpv_properties = TRUE;
		}
	}
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
			{"loop-file", PROP_LOOP_FILE, G_TYPE_STRING},
			{"loop-playlist", PROP_LOOP_PLAYLIST, G_TYPE_STRING},
			{"duration", PROP_DURATION, G_TYPE_DOUBLE},
			{"media-title", PROP_MEDIA_TITLE, G_TYPE_STRING},
			{"playlist-count", PROP_PLAYLIST_COUNT, G_TYPE_INT64},
			{"playlist-pos", PROP_PLAYLIST_POS, G_TYPE_INT64},
			{"speed", PROP_SPEED, G_TYPE_DOUBLE},
			{"volume", PROP_VOLUME, G_TYPE_DOUBLE},
			{"window-scale", PROP_WINDOW_SCALE, G_TYPE_DOUBLE},
			{"display-fps", PROP_DISPLAY_FPS, G_TYPE_DOUBLE},
			{NULL, PROP_INVALID, 0} };

	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->set_property = set_property;
	obj_class->get_property = get_property;
	obj_class->dispose = dispose;
	obj_class->finalize = finalize;

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
}

static void gmpv_model_init(GmpvModel *model)
{
	model->extra_options = NULL;
	model->ready = FALSE;
	model->cache = NULL;
	model->metadata = NULL;
	model->track_list = NULL;
	model->update_mpv_properties = TRUE;
	model->aid = NULL;
	model->vid = NULL;
	model->sid = NULL;
	model->chapters = 0;
	model->core_idle = FALSE;
	model->idle_active = FALSE;
	model->fullscreen = FALSE;
	model->pause = TRUE;
	model->loop_file = NULL;
	model->loop_playlist = NULL;
	model->duration = 0.0;
	model->media_title = NULL;
	model->playlist_count = 0;
	model->playlist_pos = 0;
	model->speed = 1.0;
	model->volume = 1.0;
	model->window_scale = 1.0;
	model->display_fps = 0.0;
}

GmpvModel *gmpv_model_new(gint64 wid)
{
	GmpvModel *model =  GMPV_MODEL(g_object_new(	gmpv_model_get_type(),
							"wid", wid,
							NULL ));

	g_signal_connect(	model,
				"mpv-property-changed",
				G_CALLBACK(mpv_prop_change_handler),
				model );

	return model;
}

void gmpv_model_initialize(GmpvModel *model, const gchar *options)
{
	GmpvMpv *mpv = GMPV_MPV(model);

	gmpv_mpv_initialize(mpv);
	gmpv_mpv_set_render_update_callback(mpv, render_update_callback, model);
}

void gmpv_model_reset(GmpvModel *model)
{
	model->ready = FALSE;
	g_object_notify(G_OBJECT(model), "ready");

	gmpv_mpv_reset(GMPV_MPV(model));
}

void gmpv_model_quit(GmpvModel *model)
{
	if(model->ready)
	{
		model->ready = FALSE;
		g_object_notify(G_OBJECT(model), "ready");

		gmpv_mpv_quit(GMPV_MPV(model));
	}
}

void gmpv_model_mouse(GmpvModel *model, gint x, gint y)
{
	gchar *x_str = g_strdup_printf("%d", x);
	gchar *y_str = g_strdup_printf("%d", y);
	const gchar *cmd[] = {"mouse", x_str, y_str, NULL};

	g_debug("Set mouse location to (%s, %s)", x_str, y_str);
	gmpv_mpv_command_async(GMPV_MPV(model), cmd);

	g_free(x_str);
	g_free(y_str);
}

void gmpv_model_key_down(GmpvModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keydown", keystr, NULL};

	g_debug("Sent '%s' key down to mpv", keystr);
	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_key_up(GmpvModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keyup", keystr, NULL};

	g_debug("Sent '%s' key up to mpv", keystr);
	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_key_press(GmpvModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keypress", keystr, NULL};

	g_debug("Sent '%s' key press to mpv", keystr);
	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_reset_keys(GmpvModel *model)
{
	const gchar *cmd[] = {"keyup", NULL};

	g_debug("Sent global key up to mpv");
	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_play(GmpvModel *model)
{
	gmpv_mpv_set_property_flag(GMPV_MPV(model), "pause", FALSE);
}

void gmpv_model_pause(GmpvModel *model)
{
	gmpv_mpv_set_property_flag(GMPV_MPV(model), "pause", TRUE);
}

void gmpv_model_stop(GmpvModel *model)
{
	const gchar *cmd[] = {"stop", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_forward(GmpvModel *model)
{
	const gchar *cmd[] = {"seek", "10", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_rewind(GmpvModel *model)
{
	const gchar *cmd[] = {"seek", "-10", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_next_chapter(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_previous_chapter(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", "down", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_next_playlist_entry(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-next", "weak", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_previous_playlist_entry(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-prev", "weak", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_shuffle_playlist(GmpvModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-shuffle", NULL};

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_seek(GmpvModel *model, gdouble value)
{
	gmpv_mpv_set_property(GMPV_MPV(model), "time-pos", MPV_FORMAT_DOUBLE, &value);
}

void gmpv_model_seek_offset(GmpvModel *model, gdouble offset)
{
	const gchar *cmd[] = {"seek", NULL, NULL};
	gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

	g_ascii_dtostr(buf, G_ASCII_DTOSTR_BUF_SIZE, offset);
	cmd[1] = buf;

	gmpv_mpv_command_async(GMPV_MPV(model), cmd);
}

void gmpv_model_load_audio_track(GmpvModel *model, const gchar *filename)
{
	gmpv_mpv_load_track
		(GMPV_MPV(model), filename, TRACK_TYPE_AUDIO);
}

void gmpv_model_load_subtitle_track(GmpvModel *model, const gchar *filename)
{
	gmpv_mpv_load_track
		(GMPV_MPV(model), filename, TRACK_TYPE_SUBTITLE);
}

gdouble gmpv_model_get_time_position(GmpvModel *model)
{
	gdouble time_pos = 0.0;

	if(!model->idle_active)
	{
		gmpv_mpv_get_property(	GMPV_MPV(model),
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&time_pos );
	}

	/* time-pos may become negative during seeks */
	return MAX(0, time_pos);
}

void gmpv_model_set_playlist_position(GmpvModel *model, gint64 position)
{
	gmpv_player_set_playlist_position(GMPV_PLAYER(model), position);
}

void gmpv_model_remove_playlist_entry(GmpvModel *model, gint64 position)
{
	gmpv_player_remove_playlist_entry(GMPV_PLAYER(model), position);
}

void gmpv_model_move_playlist_entry(GmpvModel *model, gint64 src, gint64 dst)
{
	gmpv_player_move_playlist_entry(GMPV_PLAYER(model), src, dst);
}

void gmpv_model_load_file(GmpvModel *model, const gchar *uri, gboolean append)
{
	gmpv_mpv_load(GMPV_MPV(model), uri, append);

	/* Start playing when replacing the playlist, ie. not appending, or
	 * adding the first file to the playlist.
	 */
	if(!append || model->playlist_count == 1)
	{
		gmpv_model_play(model);
	}
}

gboolean gmpv_model_get_use_opengl_cb(GmpvModel *model)
{
	return gmpv_mpv_get_use_opengl_cb(GMPV_MPV(model));
}

void gmpv_model_initialize_gl(GmpvModel *model)
{
	gmpv_mpv_init_gl(GMPV_MPV(model));
}

void gmpv_model_render_frame(GmpvModel *model, gint width, gint height)
{
	mpv_render_context *render_ctx;

	render_ctx = gmpv_mpv_get_render_context(GMPV_MPV(model));

	if(render_ctx)
	{
		gint fbo = -1;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);

		mpv_opengl_fbo opengl_fbo =
			{fbo, width, height, 0};
		mpv_render_param params[] =
			{	{MPV_RENDER_PARAM_OPENGL_FBO, &opengl_fbo},
				{MPV_RENDER_PARAM_FLIP_Y, &(int){1}},
				{0, NULL} };

		mpv_render_context_render(render_ctx, params);
	}
}

void gmpv_model_get_video_geometry(	GmpvModel *model,
					gint64 *width,
					gint64 *height )
{
	GmpvMpv *mpv = GMPV_MPV(model);

	gmpv_mpv_get_property(mpv, "dwidth", MPV_FORMAT_INT64, width);
	gmpv_mpv_get_property(mpv, "dheight", MPV_FORMAT_INT64, height);
}

gchar *gmpv_model_get_current_path(GmpvModel *model)
{
	GmpvMpv *mpv = GMPV_MPV(model);
	gchar *path = gmpv_mpv_get_property_string(mpv, "path");
	gchar *buf = g_strdup(path);

	mpv_free(path);

	return buf;
}
