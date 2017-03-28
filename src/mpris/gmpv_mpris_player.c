/*
 * Copyright (c) 2015-2017 gnome-mpv
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

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#include <mpv/client.h>
#include <string.h>

#include "gmpv_mpris_player.h"
#include "gmpv_application_private.h"
#include "gmpv_main_window.h"
#include "gmpv_mpris.h"
#include "gmpv_mpris_player.h"
#include "gmpv_mpris_gdbus.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_APP,
	N_PROPERTIES
};

struct _GmpvMprisPlayer
{
	GmpvMprisModule parent;
	GmpvApplication *app;
	guint reg_id;
};

struct _GmpvMprisPlayerClass
{
	GmpvMprisModuleClass parent_class;
};

static void register_interface(GmpvMprisModule *module);
static void unregister_interface(GmpvMprisModule *module);

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void append_metadata_tags(GVariantBuilder *builder, GSList *list);
static void method_handler(	GDBusConnection *connection,
				const gchar *sender,
				const gchar *object_path,
				const gchar *interface_name,
				const gchar *method_name,
				GVariant *parameters,
				GDBusMethodInvocation *invocation,
				gpointer data );
static GVariant *get_prop_handler(	GDBusConnection *connection,
					const gchar *sender,
					const gchar *object_path,
					const gchar *interface_name,
					const gchar *property_name,
					GError **error,
					gpointer data );
static gboolean set_prop_handler(	GDBusConnection *connection,
					const gchar *sender,
					const gchar *object_path,
					const gchar *interface_name,
					const gchar *property_name,
					GVariant *value,
					GError **error,
					gpointer data );
static void update_playback_status(GmpvMprisPlayer *player);
static void update_playlist_state(GmpvMprisPlayer *player);

static void idle_active_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data );
static void core_idle_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void playlist_pos_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data );
static void playlist_count_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data );
static void speed_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void metadata_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void volume_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static void playback_restart_handler(GmpvModel *model, gpointer data);

static void gmpv_mpris_player_class_init(GmpvMprisPlayerClass *klass);
static void gmpv_mpris_player_init(GmpvMprisPlayer *player);

G_DEFINE_TYPE(GmpvMprisPlayer, gmpv_mpris_player, GMPV_TYPE_MPRIS_MODULE);

static void register_interface(GmpvMprisModule *module)
{
	GmpvMprisPlayer *player = GMPV_MPRIS_PLAYER(module);
	GmpvModel *model = player->app->model;
	GDBusConnection *conn;
	GDBusInterfaceInfo *iface;
	GDBusInterfaceVTable vtable;

	g_object_get(module, "conn", &conn, "iface", &iface, NULL);

	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"notify::core-idle",
			G_CALLBACK(core_idle_handler),
			player );
	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"notify::idle-active",
			G_CALLBACK(idle_active_handler),
			player );
	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"notify::playlist-pos",
			G_CALLBACK(playlist_pos_handler),
			player );
	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"notify::playlist-count",
			G_CALLBACK(playlist_count_handler),
			player );
	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"notify::speed",
			G_CALLBACK(speed_handler),
			player );
	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"notify::metadata",
			G_CALLBACK(metadata_handler),
			player );
	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"notify::volume",
			G_CALLBACK(volume_handler),
			player );
	gmpv_mpris_module_connect_signal
		(	module,
			model,
			"playback-restart",
			G_CALLBACK(playback_restart_handler),
			player );

	gmpv_mpris_module_set_properties
		(	module,
			"PlaybackStatus", g_variant_new_string("Stopped"),
			"Rate", g_variant_new_double(1.0),
			"Metadata", g_variant_new("a{sv}", NULL),
			"Volume", g_variant_new_double(1.0),
			"MinimumRate", g_variant_new_double(0.01),
			"MaximumRate", g_variant_new_double(100.0),
			"CanGoNext", g_variant_new_boolean(FALSE),
			"CanGoPrevious", g_variant_new_boolean(FALSE),
			"CanPlay", g_variant_new_boolean(TRUE),
			"CanPause", g_variant_new_boolean(TRUE),
			"CanSeek", g_variant_new_boolean(FALSE),
			"CanControl", g_variant_new_boolean(TRUE),
			NULL );

	vtable.method_call = (GDBusInterfaceMethodCallFunc)method_handler;
	vtable.get_property = (GDBusInterfaceGetPropertyFunc)get_prop_handler;
	vtable.set_property = (GDBusInterfaceSetPropertyFunc)set_prop_handler;

	player->reg_id = g_dbus_connection_register_object
				(	conn,
					MPRIS_OBJ_ROOT_PATH,
					iface,
					&vtable,
					player,
					NULL,
					NULL );
}

static void unregister_interface(GmpvMprisModule *module)
{
	GmpvMprisPlayer *player = GMPV_MPRIS_PLAYER(module);
	GDBusConnection *conn = NULL;

	g_object_get(module, "conn", &conn, NULL);
	g_dbus_connection_unregister_object(conn, player->reg_id);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMprisPlayer *self = GMPV_MPRIS_PLAYER(object);

	switch(property_id)
	{
		case PROP_APP:
		self->app = g_value_get_pointer(value);
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
	GmpvMprisPlayer *self = GMPV_MPRIS_PLAYER(object);

	switch(property_id)
	{
		case PROP_APP:
		g_value_set_pointer(value, self->app);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void append_metadata_tags(GVariantBuilder *builder, GSList *list)
{
	const struct
	{
		const gchar *mpv_name;
		const gchar *tag_name;
		const gboolean is_array;
	}
	tag_map[] = {	{"Album", "xesam:album", FALSE},
			{"Album_Artist", "xesam:albumArtist", TRUE},
			{"Artist", "xesam:artist", TRUE},
			{"Comment", "xesam:comment", TRUE},
			{"Composer", "xesam:composer", FALSE},
			{"Genre", "xesam:genre", TRUE},
			{"Title", "xesam:title", FALSE},
			{NULL, NULL, FALSE} };

	for(GSList *iter = list; iter; iter = g_slist_next(iter))
	{
		const gchar *tag_name;
		GVariant *tag_value;
		GmpvMetadataEntry *entry = iter->data;
		gboolean is_array = TRUE;
		gint j = -1;

		/* Translate applicable mpv tag names to MPRIS2-compatible tag
		 * names.
		 */
		while(	tag_map[++j].mpv_name &&
			g_ascii_strcasecmp(entry->key, tag_map[j].mpv_name) != 0 );
		tag_name = tag_map[j].mpv_name?tag_map[j].tag_name:entry->key;
		is_array = tag_map[j].mpv_name?tag_map[j].is_array:FALSE;

		if(!is_array)
		{
			tag_value = g_variant_new_string(entry->value);
		}
		else if(is_array)
		{
			GVariantBuilder tag_builder;
			GVariant *elem_value;

			elem_value = g_variant_new_string(entry->value);

			g_variant_builder_init
				(&tag_builder, G_VARIANT_TYPE("as"));
			g_variant_builder_add_value
				(&tag_builder, elem_value);

			tag_value = g_variant_new("as", &tag_builder);
		}

		g_debug("Adding metadata tag \"%s\"", tag_name);
		g_variant_builder_add(builder, "{sv}", tag_name, tag_value);
	}
}

static void method_handler(	GDBusConnection *connection,
				const gchar *sender,
				const gchar *object_path,
				const gchar *interface_name,
				const gchar *method_name,
				GVariant *parameters,
				GDBusMethodInvocation *invocation,
				gpointer data )
{
	GmpvMprisPlayer *player = data;
	GmpvModel *model = player->app->model;

	if(g_strcmp0(method_name, "Next") == 0)
	{
		gmpv_model_next_playlist_entry(model);
	}
	else if(g_strcmp0(method_name, "Previous") == 0)
	{
		gmpv_model_previous_playlist_entry(model);
	}
	else if(g_strcmp0(method_name, "Pause") == 0)
	{
		gmpv_model_pause(model);
	}
	else if(g_strcmp0(method_name, "PlayPause") == 0)
	{
		gboolean pause = TRUE;

		g_object_get(G_OBJECT(model), "pause", &pause, NULL);
		(pause?gmpv_model_play:gmpv_model_pause)(model);
	}
	else if(g_strcmp0(method_name, "Stop") == 0)
	{
		gmpv_model_stop(model);
	}
	else if(g_strcmp0(method_name, "Play") == 0)
	{
		gmpv_model_play(model);
	}
	else if(g_strcmp0(method_name, "Seek") == 0)
	{
		gint64 offset_us;

		g_variant_get(parameters, "(x)", &offset_us);
		gmpv_model_seek_offset(model, (gdouble)offset_us/1.0e6);
	}
	else if(g_strcmp0(method_name, "SetPosition") == 0)
	{
		const gchar *prefix = MPRIS_TRACK_ID_PREFIX;
		const gsize prefix_len = strlen(prefix);
		gint64 time_us = -1;
		const gchar *track_id = NULL;

		g_variant_get(parameters, "(&ox)", &track_id, &time_us);

		if(strncmp(track_id, prefix, prefix_len) == 0)
		{
			gint64 index =	g_ascii_strtoll
					(track_id+prefix_len, NULL, 0);

			gmpv_model_set_playlist_position(model, index);
			gmpv_model_seek(model, (gdouble)time_us/1.0e6);
		}
	}
	else if(g_strcmp0(method_name, "OpenUri") == 0)
	{
		const gchar *uri;

		g_variant_get(parameters, "(&s)", &uri);
		gmpv_model_load_file(model, uri, FALSE);
	}

	g_dbus_method_invocation_return_value
		(invocation, g_variant_new("()", NULL));
}

static GVariant *get_prop_handler(	GDBusConnection *connection,
					const gchar *sender,
					const gchar *object_path,
					const gchar *interface_name,
					const gchar *property_name,
					GError **error,
					gpointer data )
{
	GmpvMprisPlayer *player = data;
	GVariant *value;

	if(g_strcmp0(property_name, "Position") == 0)
	{
		GmpvModel *model;
		gdouble position;

		model = player->app->model;
		position = gmpv_model_get_time_position(model);
		value = g_variant_new_int64((gint64)(position*1e6));
	}
	else
	{
		gmpv_mpris_module_get_properties(	GMPV_MPRIS_MODULE(data),
							property_name, &value,
							NULL );
	}

	return value?g_variant_ref(value):NULL;
}

static gboolean set_prop_handler(	GDBusConnection *connection,
					const gchar *sender,
					const gchar *object_path,
					const gchar *interface_name,
					const gchar *property_name,
					GVariant *value,
					GError **error,
					gpointer data )
{
	GmpvMprisPlayer *player = data;
	GmpvModel *model = player->app->model;

	if(g_strcmp0(property_name, "Rate") == 0)
	{
		g_object_set(	G_OBJECT(model),
				"speed", g_variant_get_double(value),
				NULL );
	}
	else if(g_strcmp0(property_name, "Volume") == 0)
	{
		g_object_set(	G_OBJECT(model),
				"volume", 100*g_variant_get_double(value),
				NULL );
	}

	gmpv_mpris_module_set_properties(	GMPV_MPRIS_MODULE(data),
						property_name, value,
						NULL );

	return TRUE; /* This function should always succeed */
}

static void update_playback_status(GmpvMprisPlayer *player)
{
	GmpvModel *model = player->app->model;
	const gchar *state;
	gint idle_active;
	gint core_idle;
	gboolean can_seek;

	g_object_get(	G_OBJECT(model),
			"idle-active", &idle_active,
			"core-idle", &core_idle,
			NULL );

	if(!core_idle && !idle_active)
	{
		state = "Playing";
		can_seek = TRUE;
	}
	else if(core_idle && idle_active)
	{
		state = "Stopped";
		can_seek = FALSE;
	}
	else
	{
		state = "Paused";
		can_seek = TRUE;
	}

	gmpv_mpris_module_set_properties(	GMPV_MPRIS_MODULE(player),
						"PlaybackStatus",
						g_variant_new_string(state),
						"CanSeek",
						g_variant_new_boolean(can_seek),
						NULL );
}

static void update_playlist_state(GmpvMprisPlayer *player)
{
	GmpvModel *model = player->app->model;
	gboolean can_prev;
	gboolean can_next;
	gint64 playlist_count;
	gint64 playlist_pos;
	gint rc = 0;

	g_object_get(	G_OBJECT(model),
			"playlist-count", &playlist_count,
			"playlist-pos", &playlist_pos,
			NULL );

	can_prev = (rc >= 0 && playlist_pos > 0);
	can_next = (rc >= 0 && playlist_pos < playlist_count-1);

	gmpv_mpris_module_set_properties(	GMPV_MPRIS_MODULE(player),
						"CanGoPrevious",
						g_variant_new_boolean(can_prev),
						"CanGoNext",
						g_variant_new_boolean(can_next),
						NULL );
}

static void idle_active_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	update_playback_status(data);
}

static void core_idle_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	update_playback_status(data);
}

static void playlist_pos_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	update_playlist_state(data);
}

static void playlist_count_handler(	GObject *object,
					GParamSpec *pspec,
					gpointer data )
{
	update_playlist_state(data);
}

static void speed_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	gdouble speed = 1.0;

	g_object_get(object, "speed", &speed, NULL);

	gmpv_mpris_module_set_properties(	GMPV_MPRIS_MODULE(data),
						"Rate",
						g_variant_new_double(speed),
						NULL );
}

static void metadata_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvModel *model = GMPV_MODEL(object);
	GSList *metadata = NULL;
	GVariantBuilder builder;
	gchar *path;
	gchar *uri;
	gchar *playlist_pos_str;
	gchar *trackid;
	gdouble duration = 0;
	gint64 playlist_pos = 0;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
	path = gmpv_model_get_current_path(model);
	uri = g_filename_to_uri(path, NULL, NULL)?:g_strdup(path);

	if(uri)
	{
		g_variant_builder_add(	&builder,
					"{sv}",
					"xesam:url",
					g_variant_new_string(uri) );

	}

	g_object_get(	model,
			"duration", &duration,
			"playlist-pos", &playlist_pos,
			"metadata", &metadata,
			NULL );

	g_variant_builder_add(	&builder,
				"{sv}",
				"mpris:length",
				g_variant_new_int64
				((gint64)(duration*1e6)) );

	playlist_pos_str = g_strdup_printf("%" G_GINT64_FORMAT, playlist_pos);
	trackid = g_strconcat(	MPRIS_TRACK_ID_PREFIX,
				playlist_pos_str,
				NULL );
	g_variant_builder_add(	&builder,
				"{sv}",
				"mpris:trackid",
				g_variant_new_object_path(trackid) );

	append_metadata_tags(&builder, metadata);

	gmpv_mpris_module_set_properties(	GMPV_MPRIS_MODULE(data),
						"Metadata",
						g_variant_new("a{sv}", &builder),
						NULL );

	g_free(path);
	g_free(uri);
	g_free(playlist_pos_str);
	g_free(trackid);
}

static void volume_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	gdouble volume = 0.0;

	g_object_get(object, "volume", &volume, NULL);

	gmpv_mpris_module_set_properties
		(	GMPV_MPRIS_MODULE(data),
			"Volume",
			g_variant_new_double(volume/100.0),
			NULL );
}

static void playback_restart_handler(GmpvModel *model, gpointer data)
{
	GDBusConnection *conn;
	GDBusInterfaceInfo *iface;
	gdouble position;

	position = gmpv_model_get_time_position(model);

	g_object_get(	GMPV_MPRIS_MODULE(data),
			"conn", &conn,
			"iface", &iface,
			NULL );

	g_dbus_connection_emit_signal
		(	conn,
			NULL,
			MPRIS_OBJ_ROOT_PATH,
			iface->name,
			"Seeked",
			g_variant_new("(x)", (gint64)(position*1e6)),
			NULL );
}

static void gmpv_mpris_player_class_init(GmpvMprisPlayerClass *klass)
{
	GmpvMprisModuleClass *module_class = GMPV_MPRIS_MODULE_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	module_class->register_interface = register_interface;
	module_class->unregister_interface = unregister_interface;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_pointer
		(	"app",
			"Application",
			"The GmpvApplication to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_APP, pspec);
}

static void gmpv_mpris_player_init(GmpvMprisPlayer *player)
{
	player->app = NULL;
	player->reg_id = 0;
}

GmpvMprisPlayer *gmpv_mpris_player_new(	GmpvApplication *app,
					GDBusConnection *conn )
{
	GDBusInterfaceInfo *iface;

	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();

	return GMPV_MPRIS_PLAYER(g_object_new(	gmpv_mpris_player_get_type(),
						"app", app,
						"conn", conn,
						"iface", iface,
						NULL ));
}

