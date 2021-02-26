/*
 * Copyright (c) 2017-2021 gnome-mpv
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

#include "celluloid-model.h"
#include "celluloid-marshal.h"
#include "celluloid-mpv-wrapper.h"
#include "celluloid-option-parser.h"
#include "celluloid-def.h"

enum
{
	PROP_INVALID,
	PROP_AID,
	PROP_VID,
	PROP_SID,
	PROP_CHAPTERS,
	PROP_CORE_IDLE,
	PROP_IDLE_ACTIVE,
	PROP_BORDER,
	PROP_FULLSCREEN,
	PROP_PAUSE,
	PROP_LOOP_FILE,
	PROP_LOOP_PLAYLIST,
	PROP_SHUFFLE,
	PROP_DURATION,
	PROP_MEDIA_TITLE,
	PROP_PLAYLIST_COUNT,
	PROP_PLAYLIST_POS,
	PROP_SPEED,
	PROP_VOLUME,
	PROP_VOLUME_MAX,
	PROP_WINDOW_MAXIMIZED,
	PROP_WINDOW_SCALE,
	PROP_DISPLAY_FPS,
	N_PROPERTIES
};

struct _CelluloidModel
{
	CelluloidPlayer parent;
	gchar *extra_options;
	GPtrArray *metadata;
	GPtrArray *track_list;
	gboolean update_mpv_properties;
	gboolean resetting;
	gchar *aid;
	gchar *vid;
	gchar *sid;
	gint64 chapters;
	gboolean core_idle;
	gboolean idle_active;
	gboolean border;
	gboolean fullscreen;
	gboolean pause;
	gchar *loop_file;
	gchar *loop_playlist;
	gboolean shuffle;
	gdouble duration;
	gchar *media_title;
	gint64 playlist_count;
	gint64 playlist_pos;
	gdouble speed;
	gdouble volume;
	gdouble volume_max;
	gboolean window_maximized;
	gdouble window_scale;
	gdouble display_fps;
};

struct _CelluloidModelClass
{
	GObjectClass parent_class;
};

static gboolean
extra_options_contains(CelluloidModel *model, const gchar *option);

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec );

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec );

static void
dispose(GObject *object);

static void
finalize(GObject *object);

static void
set_mpv_property(	GObject *object,
			guint property_id,
			const GValue *value,
			GParamSpec *pspec );

static void
g_value_set_by_type(GValue *gvalue, GType type, gpointer value);

static GParamSpec *
g_param_spec_by_type(	const gchar *name,
			const gchar *nick,
			const gchar *blurb,
			GType type,
			GParamFlags flags );

static gboolean
emit_frame_ready(gpointer data);

static void
render_update_callback(gpointer render_ctx);

static void
mpv_prop_change_handler(	CelluloidMpv *mpv,
				const gchar *name,
				gpointer value,
				gpointer data );

G_DEFINE_TYPE(CelluloidModel, celluloid_model, CELLULOID_TYPE_PLAYER)

static gboolean
extra_options_contains(CelluloidModel *model, const gchar *option)
{
	gboolean result = FALSE;
	gchar *extra_options = NULL;
	const gchar *cur = NULL;

	g_object_get(model, "extra-options", &extra_options, NULL);

	cur = extra_options;

	while(cur && *cur && !result)
	{
		gchar *key = NULL;
		gchar *value = NULL;

		cur = parse_option(cur, &key, &value);

		if(key && *key)
		{
			result |= g_strcmp0(key, option) == 0;
		}
		else
		{
			g_warning("Failed to parse options");

			cur = NULL;
		}

		g_free(key);
		g_free(value);
	}

	g_free(extra_options);

	return result;
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidModel *self = CELLULOID_MODEL(object);

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

		if(self->resetting)
		{
			if(self->pause)
			{
				celluloid_model_pause(self);
			}
			else
			{
				celluloid_model_play(self);
			}

			self->resetting = FALSE;
		}
		break;

		case PROP_IDLE_ACTIVE:
		self->idle_active = g_value_get_boolean(value);

		if(self->idle_active)
		{
			g_object_notify(object, "playlist-pos");
		}
		break;

		case PROP_BORDER:
		self->border = g_value_get_boolean(value);
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

		case PROP_SHUFFLE:
		{
			gboolean ready = FALSE;

			self->shuffle = g_value_get_boolean(value);
			g_object_get(self, "ready", &ready, NULL);

			if(ready)
			{
				if(self->shuffle)
				{
					celluloid_model_shuffle_playlist(self);
				}
				else
				{
					celluloid_model_unshuffle_playlist(self);
				}
			}
		}
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

		case PROP_VOLUME_MAX:
		self->volume_max = g_value_get_double(value);
		break;

		case PROP_WINDOW_MAXIMIZED:
		self->window_maximized = g_value_get_boolean(value);
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

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidModel *self = CELLULOID_MODEL(object);

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

		case PROP_BORDER:
		g_value_set_boolean(value, self->border);
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

		case PROP_SHUFFLE:
		g_value_set_boolean(value, self->shuffle);
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

		case PROP_VOLUME_MAX:
		g_value_set_double(value, self->volume_max);
		break;

		case PROP_WINDOW_MAXIMIZED:
		g_value_set_boolean(value, self->window_maximized);
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

static void
dispose(GObject *object)
{
	CelluloidModel *model = CELLULOID_MODEL(object);
	CelluloidMpv *mpv = CELLULOID_MPV(model);

	if(mpv)
	{
		celluloid_mpv_set_render_update_callback(mpv, NULL, NULL);
		while(g_source_remove_by_user_data(model));
	}

	g_free(model->extra_options);

	G_OBJECT_CLASS(celluloid_model_parent_class)->dispose(object);
}

static void finalize(GObject *object)
{
	CelluloidModel *model = CELLULOID_MODEL(object);

	g_free(model->aid);
	g_free(model->vid);
	g_free(model->sid);
	g_free(model->loop_file);
	g_free(model->loop_playlist);
	g_free(model->media_title);

	G_OBJECT_CLASS(celluloid_model_parent_class)->finalize(object);
}

static void
set_mpv_property(	GObject *object,
			guint property_id,
			const GValue *value,
			GParamSpec *pspec )
{
	CelluloidModel *self = CELLULOID_MODEL(object);
	CelluloidMpv *mpv = CELLULOID_MPV(self);

	switch(property_id)
	{
		case PROP_AID:
		celluloid_mpv_set_property(	mpv,
						"aid",
						MPV_FORMAT_STRING,
						&self->aid );
		break;

		case PROP_VID:
		celluloid_mpv_set_property(	mpv,
						"vid",
						MPV_FORMAT_STRING,
						&self->vid );
		break;

		case PROP_SID:
		celluloid_mpv_set_property(	mpv,
						"sid",
						MPV_FORMAT_STRING,
						&self->sid );
		break;

		case PROP_BORDER:
		celluloid_mpv_set_property(	mpv,
						"border",
						MPV_FORMAT_FLAG,
						&self->border );
		break;

		case PROP_FULLSCREEN:
		celluloid_mpv_set_property(	mpv,
						"fullscreen",
						MPV_FORMAT_FLAG,
						&self->fullscreen );
		break;

		case PROP_PAUSE:
		celluloid_mpv_set_property(	mpv,
						"pause",
						MPV_FORMAT_FLAG,
						&self->pause );
		break;

		case PROP_LOOP_FILE:
		celluloid_mpv_set_property(	mpv,
						"loop-file",
						MPV_FORMAT_STRING,
						&self->loop_file );
		break;

		case PROP_LOOP_PLAYLIST:
		celluloid_mpv_set_property(	mpv,
						"loop-playlist",
						MPV_FORMAT_STRING,
						&self->loop_playlist );
		break;

		case PROP_MEDIA_TITLE:
		celluloid_mpv_set_property(	mpv,
						"media-title",
						MPV_FORMAT_INT64,
						&self->media_title );
		break;

		case PROP_PLAYLIST_POS:
		celluloid_mpv_set_property(	mpv,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&self->playlist_pos );
		break;

		case PROP_SPEED:
		celluloid_mpv_set_property(	mpv,
						"speed",
						MPV_FORMAT_DOUBLE,
						&self->speed );
		break;

		case PROP_VOLUME:
		celluloid_mpv_set_property(	mpv,
						"volume",
						MPV_FORMAT_DOUBLE,
						&self->volume );
		break;

		case PROP_VOLUME_MAX:
		celluloid_mpv_set_property(	mpv,
						"volume-max",
						MPV_FORMAT_DOUBLE,
						&self->volume_max );
		break;

		case PROP_WINDOW_MAXIMIZED:
		celluloid_mpv_set_property(	mpv,
						"window-maximized",
						MPV_FORMAT_FLAG,
						&self->window_maximized );
		break;

		case PROP_WINDOW_SCALE:
		celluloid_mpv_set_property(	mpv,
						"window-scale",
						MPV_FORMAT_DOUBLE,
						&self->window_scale );
		break;

		case PROP_DISPLAY_FPS:
		celluloid_mpv_set_property(	mpv,
						"display-fps",
						MPV_FORMAT_DOUBLE,
						&self->display_fps );
		break;
	}
}

static void
g_value_set_by_type(GValue *gvalue, GType type, gpointer value)
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

static GParamSpec *
g_param_spec_by_type(	const gchar *name,
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

static gboolean
emit_frame_ready(gpointer data)
{
	CelluloidModel *model = data;
	guint64 flags = celluloid_mpv_render_context_update(CELLULOID_MPV(model));

	if(flags&MPV_RENDER_UPDATE_FRAME)
	{
		g_signal_emit_by_name(model, "frame-ready");
	}

	return FALSE;
}

static void
render_update_callback(gpointer data)
{
	g_idle_add_full(	G_PRIORITY_HIGH,
				emit_frame_ready,
				data,
				NULL );
}

static void
mpv_prop_change_handler(	CelluloidMpv *mpv,
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
			(data, CELLULOID_TYPE_MODEL, GObjectClass);
		pspec = g_object_class_find_property(klass, name);

		if(pspec && value)
		{
			CELLULOID_MODEL(data)->update_mpv_properties = FALSE;

			g_value_set_by_type(&gvalue, pspec->value_type, value);
			g_object_set_property(data, name, &gvalue);

			CELLULOID_MODEL(data)->update_mpv_properties = TRUE;
		}
	}
}

static void
celluloid_model_class_init(CelluloidModelClass *klass)
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
			{"border", PROP_BORDER, G_TYPE_BOOLEAN},
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
			{"volume-max", PROP_VOLUME_MAX, G_TYPE_DOUBLE},
			{"window-maximized", PROP_WINDOW_MAXIMIZED, G_TYPE_BOOLEAN},
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

	pspec = g_param_spec_boolean
		(	"shuffle",
			"Shuffle",
			"Whether or not the playlist is shuffled",
			FALSE,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_SHUFFLE, pspec);

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

static void
celluloid_model_init(CelluloidModel *model)
{
	model->extra_options = NULL;
	model->metadata = NULL;
	model->track_list = NULL;
	model->update_mpv_properties = TRUE;
	model->resetting = FALSE;
	model->aid = NULL;
	model->vid = NULL;
	model->sid = NULL;
	model->chapters = 0;
	model->core_idle = FALSE;
	model->idle_active = FALSE;
	model->border = FALSE;
	model->fullscreen = FALSE;
	model->pause = TRUE;
	model->loop_file = NULL;
	model->loop_playlist = NULL;
	model->shuffle = FALSE;
	model->duration = 0.0;
	model->media_title = NULL;
	model->playlist_count = 0;
	model->playlist_pos = 0;
	model->speed = 1.0;
	model->volume = 1.0;
	model->volume_max = 100.0;
	model->window_maximized = FALSE;
	model->window_scale = 1.0;
	model->display_fps = 0.0;
}

CelluloidModel *
celluloid_model_new(gint64 wid)
{
	const GType type = celluloid_model_get_type();
	CelluloidModel *model =  CELLULOID_MODEL(g_object_new(	type,
								"wid", wid,
								NULL ));

	g_signal_connect(	model,
				"mpv-property-changed",
				G_CALLBACK(mpv_prop_change_handler),
				model );

	return model;
}

void
celluloid_model_initialize(CelluloidModel *model)
{
	CelluloidMpv *mpv = CELLULOID_MPV(model);

	celluloid_mpv_initialize
		(mpv);
	celluloid_mpv_set_render_update_callback
		(mpv, render_update_callback, model);

	if(!extra_options_contains(model, "volume"))
	{
		GSettings *win_settings = g_settings_new(CONFIG_WIN_STATE);
		gdouble volume = g_settings_get_double(win_settings, "volume")*100;

		g_debug("Setting volume to %f", volume);

		celluloid_mpv_set_property
			(mpv, "volume", MPV_FORMAT_DOUBLE, &volume);

		g_object_unref(win_settings);
	}
}

void
celluloid_model_reset(CelluloidModel *model)
{
	model->resetting = TRUE;

	celluloid_mpv_reset(CELLULOID_MPV(model));
}

void
celluloid_model_quit(CelluloidModel *model)
{
	celluloid_mpv_quit(CELLULOID_MPV(model));
}

void
celluloid_model_mouse(CelluloidModel *model, gint x, gint y)
{
	gchar *x_str = g_strdup_printf("%d", x);
	gchar *y_str = g_strdup_printf("%d", y);
	const gchar *cmd[] = {"mouse", x_str, y_str, NULL};

	g_debug("Set mouse location to (%s, %s)", x_str, y_str);
	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);

	g_free(x_str);
	g_free(y_str);
}

void
celluloid_model_key_down(CelluloidModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keydown", keystr, NULL};

	g_debug("Sent '%s' key down to mpv", keystr);
	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_key_up(CelluloidModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keyup", keystr, NULL};

	g_debug("Sent '%s' key up to mpv", keystr);
	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_key_press(CelluloidModel *model, const gchar* keystr)
{
	const gchar *cmd[] = {"keypress", keystr, NULL};

	g_debug("Sent '%s' key press to mpv", keystr);
	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_reset_keys(CelluloidModel *model)
{
	// As of Sep 20, 2019, mpv will crash if the command doesn't have any
	// arguments. The empty string is there to work around the issue.
	const gchar *cmd[] = {"keyup", "", NULL};

	g_debug("Sent global key up to mpv");
	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_play(CelluloidModel *model)
{
	celluloid_mpv_set_property_flag(CELLULOID_MPV(model), "pause", FALSE);
}

void
celluloid_model_pause(CelluloidModel *model)
{
	celluloid_mpv_set_property_flag(CELLULOID_MPV(model), "pause", TRUE);
}

void
celluloid_model_stop(CelluloidModel *model)
{
	const gchar *cmd[] = {"stop", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_forward(CelluloidModel *model)
{
	const gchar *cmd[] = {"seek", "10", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_rewind(CelluloidModel *model)
{
	const gchar *cmd[] = {"seek", "-10", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_next_chapter(CelluloidModel *model)
{
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_previous_chapter(CelluloidModel *model)
{
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", "down", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_next_playlist_entry(CelluloidModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-next", "weak", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_previous_playlist_entry(CelluloidModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-prev", "weak", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_shuffle_playlist(CelluloidModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-shuffle", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_unshuffle_playlist(CelluloidModel *model)
{
	const gchar *cmd[] = {"osd-msg", "playlist-unshuffle", NULL};

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_seek(CelluloidModel *model, gdouble value)
{
	celluloid_mpv_set_property(CELLULOID_MPV(model), "time-pos", MPV_FORMAT_DOUBLE, &value);
}

void
celluloid_model_seek_offset(CelluloidModel *model, gdouble offset)
{
	const gchar *cmd[] = {"seek", NULL, NULL};
	gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

	g_ascii_dtostr(buf, G_ASCII_DTOSTR_BUF_SIZE, offset);
	cmd[1] = buf;

	celluloid_mpv_command_async(CELLULOID_MPV(model), cmd);
}

void
celluloid_model_load_audio_track(CelluloidModel *model, const gchar *filename)
{
	celluloid_mpv_load_track
		(CELLULOID_MPV(model), filename, TRACK_TYPE_AUDIO);
}

void
celluloid_model_load_video_track(CelluloidModel *model, const gchar *filename)
{
	celluloid_mpv_load_track
		(CELLULOID_MPV(model), filename, TRACK_TYPE_VIDEO);
}

void
celluloid_model_load_subtitle_track(	CelluloidModel *model,
					const gchar *filename )
{
	celluloid_mpv_load_track
		(CELLULOID_MPV(model), filename, TRACK_TYPE_SUBTITLE);
}

gdouble
celluloid_model_get_time_position(CelluloidModel *model)
{
	gdouble time_pos = 0.0;

	if(!model->idle_active)
	{
		celluloid_mpv_get_property(	CELLULOID_MPV(model),
						"time-pos",
						MPV_FORMAT_DOUBLE,
						&time_pos );
	}

	/* time-pos may become negative during seeks */
	return MAX(0, time_pos);
}

void
celluloid_model_set_playlist_position(CelluloidModel *model, gint64 position)
{
	celluloid_player_set_playlist_position
		(CELLULOID_PLAYER(model), position);
}

void
celluloid_model_remove_playlist_entry(CelluloidModel *model, gint64 position)
{
	celluloid_player_remove_playlist_entry
		(CELLULOID_PLAYER(model), position);
}

void
celluloid_model_move_playlist_entry(	CelluloidModel *model,
					gint64 src,
					gint64 dst )
{
	celluloid_player_move_playlist_entry(CELLULOID_PLAYER(model), src, dst);
}

void
celluloid_model_load_file(	CelluloidModel *model,
				const gchar *uri,
				gboolean append )
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	append |= g_settings_get_boolean(settings, "always-append-to-playlist");

	celluloid_mpv_load(CELLULOID_MPV(model), uri, append);

	/* Start playing when replacing the playlist, ie. not appending, or
	 * adding the first file to the playlist.
	 */
	if(!append || model->playlist_count == 0)
	{
		celluloid_model_play(model);
	}

	g_object_unref(settings);
}

gboolean
celluloid_model_get_use_opengl_cb(CelluloidModel *model)
{
	return celluloid_mpv_get_use_opengl_cb(CELLULOID_MPV(model));
}

void
celluloid_model_initialize_gl(CelluloidModel *model)
{
	celluloid_mpv_init_gl(CELLULOID_MPV(model));
}

void
celluloid_model_render_frame(CelluloidModel *model, gint width, gint height)
{
	mpv_render_context *render_ctx;

	render_ctx = celluloid_mpv_get_render_context(CELLULOID_MPV(model));

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

void
celluloid_model_get_video_geometry(	CelluloidModel *model,
					gint64 *width,
					gint64 *height )
{
	CelluloidMpv *mpv = CELLULOID_MPV(model);

	celluloid_mpv_get_property(mpv, "dwidth", MPV_FORMAT_INT64, width);
	celluloid_mpv_get_property(mpv, "dheight", MPV_FORMAT_INT64, height);
}

gchar *
celluloid_model_get_current_path(CelluloidModel *model)
{
	CelluloidMpv *mpv = CELLULOID_MPV(model);
	gchar *path = celluloid_mpv_get_property_string(mpv, "path");
	gchar *buf = g_strdup(path);

	mpv_free(path);

	return buf;
}
