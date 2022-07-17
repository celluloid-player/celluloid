/*
 * Copyright (c) 2015-2019, 2021-2022 gnome-mpv
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

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>
#include <mpv/client.h>
#include <string.h>

#include "celluloid-mpris-player.h"
#include "celluloid-common.h"
#include "celluloid-main-window.h"
#include "celluloid-mpris.h"
#include "celluloid-mpris-player.h"
#include "celluloid-mpris-gdbus.h"
#include "celluloid-def.h"

enum
{
	PROP_0,
	PROP_CONTROLLER,
	N_PROPERTIES
};

struct _CelluloidMprisPlayer
{
	CelluloidMprisModule parent;
	CelluloidController *controller;
	GHashTable *readonly_table;
	guint reg_id;
};

struct _CelluloidMprisPlayerClass
{
	CelluloidMprisModuleClass parent_class;
};

static void
register_interface(CelluloidMprisModule *module);

static void
unregister_interface(CelluloidMprisModule *module);

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
append_metadata_tags(GVariantBuilder *builder, GPtrArray *list);

static void
method_handler(	GDBusConnection *connection,
		const gchar *sender,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		gpointer data );

static GVariant *
get_prop_handler(	GDBusConnection *connection,
			const gchar *sender,
			const gchar *object_path,
			const gchar *interface_name,
			const gchar *property_name,
			GError **error,
			gpointer data );

static gboolean
set_prop_handler(	GDBusConnection *connection,
			const gchar *sender,
			const gchar *object_path,
			const gchar *interface_name,
			const gchar *property_name,
			GVariant *value,
			GError **error,
			gpointer data );

static void
update_playback_status(CelluloidMprisPlayer *player);

static void
update_playlist_state(CelluloidMprisPlayer *player);

static void
update_speed(CelluloidMprisPlayer *player);

static void
update_loop(CelluloidMprisPlayer *player);

static void
update_metadata(CelluloidMprisPlayer *player);

static void
update_volume(CelluloidMprisPlayer *player);

static void
idle_active_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data );

static void
core_idle_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data );

static void
playlist_pos_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data );

static void
playlist_count_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data );
static void
speed_handler(	GObject *object,
		GParamSpec *pspec,
		gpointer data );

static void
loop_handler(	GObject *object,
		GParamSpec *pspec,
		gpointer data );

static void
metadata_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data );

static void
volume_handler(	GObject *object,
		GParamSpec *pspec,
		gpointer data );

static void
playback_restart_handler(CelluloidModel *model, gpointer data);

static void
celluloid_mpris_player_class_init(CelluloidMprisPlayerClass *klass);

static void
celluloid_mpris_player_init(CelluloidMprisPlayer *player);

G_DEFINE_TYPE(CelluloidMprisPlayer, celluloid_mpris_player, CELLULOID_TYPE_MPRIS_MODULE);

static void
register_interface(CelluloidMprisModule *module)
{
	CelluloidMprisPlayer *player = CELLULOID_MPRIS_PLAYER(module);
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);

	GDBusConnection *conn;
	GDBusInterfaceInfo *iface;
	GDBusInterfaceVTable vtable;

	g_object_get(module, "conn", &conn, "iface", &iface, NULL);

	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::core-idle",
			G_CALLBACK(core_idle_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::idle-active",
			G_CALLBACK(idle_active_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::playlist-pos",
			G_CALLBACK(playlist_pos_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::playlist-count",
			G_CALLBACK(playlist_count_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::speed",
			G_CALLBACK(speed_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::loop-file",
			G_CALLBACK(loop_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::loop-playlist",
			G_CALLBACK(loop_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::metadata",
			G_CALLBACK(metadata_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::playlist-pos",
			G_CALLBACK(metadata_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"notify::volume",
			G_CALLBACK(volume_handler),
			player );
	celluloid_mpris_module_connect_signal
		(	module,
			model,
			"playback-restart",
			G_CALLBACK(playback_restart_handler),
			player );

	celluloid_mpris_module_set_properties
		(	module,
			"PlaybackStatus", g_variant_new_string("Stopped"),
			"LoopStatus", g_variant_new_string("None"),
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

	update_playback_status(player);
	update_playlist_state(player);
	update_speed(player);
	update_metadata(player);
	update_volume(player);
}

static void
unregister_interface(CelluloidMprisModule *module)
{
	CelluloidMprisPlayer *player = CELLULOID_MPRIS_PLAYER(module);
	GDBusConnection *conn = NULL;

	g_object_get(module, "conn", &conn, NULL);
	g_dbus_connection_unregister_object(conn, player->reg_id);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidMprisPlayer *self = CELLULOID_MPRIS_PLAYER(object);

	switch(property_id)
	{
		case PROP_CONTROLLER:
		self->controller = g_value_get_pointer(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidMprisPlayer *self = CELLULOID_MPRIS_PLAYER(object);

	switch(property_id)
	{
		case PROP_CONTROLLER:
		g_value_set_pointer(value, self->controller);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
append_metadata_tags(GVariantBuilder *builder, GPtrArray *list)
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

	const guint list_len = list?list->len:0;

	for(guint i = 0; i < list_len; i++)
	{
		const gchar *tag_name;
		GVariant *tag_value;
		CelluloidMetadataEntry *entry = g_ptr_array_index(list, i);
		gboolean is_array = TRUE;
		gint j = -1;

		/* Translate applicable mpv tag names to MPRIS2-compatible tag
		 * names.
		 */
		while(	tag_map[++j].mpv_name &&
			g_ascii_strcasecmp(entry->key, tag_map[j].mpv_name) != 0 );
		tag_name = tag_map[j].mpv_name?tag_map[j].tag_name:entry->key;
		is_array = tag_map[j].mpv_name?tag_map[j].is_array:FALSE;

		if(is_array)
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
		else
		{
			tag_value = g_variant_new_string(entry->value);
		}

		g_debug("Adding metadata tag \"%s\"", tag_name);
		g_variant_builder_add(builder, "{sv}", tag_name, tag_value);
	}
}

static void
method_handler(	GDBusConnection *connection,
		const gchar *sender,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		gpointer data )
{
	CelluloidMprisPlayer *player = data;
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
	gboolean unknown_method = FALSE;

	if(g_strcmp0(method_name, "Next") == 0)
	{
		celluloid_model_next_playlist_entry(model);
	}
	else if(g_strcmp0(method_name, "Previous") == 0)
	{
		celluloid_model_previous_playlist_entry(model);
	}
	else if(g_strcmp0(method_name, "Pause") == 0)
	{
		celluloid_model_pause(model);
	}
	else if(g_strcmp0(method_name, "PlayPause") == 0)
	{
		gboolean pause = FALSE;

		g_object_get(model, "pause", &pause, NULL);
		g_object_set(model, "pause", !pause, NULL);
	}
	else if(g_strcmp0(method_name, "Stop") == 0)
	{
		celluloid_model_stop(model);
	}
	else if(g_strcmp0(method_name, "Play") == 0)
	{
		celluloid_model_play(model);
	}
	else if(g_strcmp0(method_name, "Seek") == 0)
	{
		gint64 offset_us;

		g_variant_get(parameters, "(x)", &offset_us);
		celluloid_model_seek_offset(model, (gdouble)offset_us/1.0e6);
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

			celluloid_model_set_playlist_position(model, index);
			celluloid_model_seek(model, (gdouble)time_us/1.0e6);
		}
	}
	else if(g_strcmp0(method_name, "OpenUri") == 0)
	{
		const gchar *uri;

		g_variant_get(parameters, "(&s)", &uri);
		celluloid_model_load_file(model, uri, FALSE);
	}
	else
	{
		unknown_method = TRUE;
	}

	if(unknown_method)
	{
		g_dbus_method_invocation_return_error
			(	invocation,
				CELLULOID_MPRIS_ERROR,
				CELLULOID_MPRIS_ERROR_UNKNOWN_METHOD,
				"Attempted to call unknown method \"%s\"",
				method_name );
	}
	else
	{
		g_dbus_method_invocation_return_value
			(invocation, g_variant_new("()", NULL));
	}
}

static GVariant *
get_prop_handler(	GDBusConnection *connection,
			const gchar *sender,
			const gchar *object_path,
			const gchar *interface_name,
			const gchar *property_name,
			GError **error,
			gpointer data )
{
	CelluloidMprisPlayer *player = CELLULOID_MPRIS_PLAYER(data);
	CelluloidMprisModule *module = CELLULOID_MPRIS_MODULE(data);
	GVariant *value = NULL;

	if(!g_hash_table_contains(player->readonly_table, property_name))
	{
		g_set_error
			(	error,
				CELLULOID_MPRIS_ERROR,
				CELLULOID_MPRIS_ERROR_UNKNOWN_PROPERTY,
				"Failed to get value of unknown property \"%s\"",
				property_name );
	}
	else if(g_strcmp0(property_name, "Position") == 0)
	{
		CelluloidModel *model;
		gdouble position;

		model = celluloid_controller_get_model(player->controller);
		position = celluloid_model_get_time_position(model);
		value = g_variant_new_int64((gint64)(position*1e6));
	}
	else
	{
		celluloid_mpris_module_get_properties
			(	module,
				property_name, &value,
				NULL );
	}

	return value?g_variant_ref(value):NULL;
}

static gboolean
set_prop_handler(	GDBusConnection *connection,
			const gchar *sender,
			const gchar *object_path,
			const gchar *interface_name,
			const gchar *property_name,
			GVariant *value,
			GError **error,
			gpointer data )
{
	CelluloidMprisPlayer *player = CELLULOID_MPRIS_PLAYER(data);
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
	gboolean result = TRUE;

	if(!g_hash_table_contains(player->readonly_table, property_name))
	{
		result = FALSE;

		g_set_error
			(	error,
				CELLULOID_MPRIS_ERROR,
				CELLULOID_MPRIS_ERROR_UNKNOWN_PROPERTY,
				"Failed to set value of unknown property \"%s\"",
				property_name );
	}
	else if(GPOINTER_TO_INT(g_hash_table_lookup(player->readonly_table, property_name)))
	{
		g_set_error
			(	error,
				CELLULOID_MPRIS_ERROR,
				CELLULOID_MPRIS_ERROR_SET_READONLY,
				"Attempted to set value of readonly property \"%s\"",
				property_name );
	}
	else if(g_strcmp0(property_name, "LoopStatus") == 0)
	{
		const gchar *loop = g_variant_get_string(value, NULL);
		const gchar *loop_file =	g_strcmp0(loop, "Track") == 0 ?
						"inf":"no";
		const gchar *loop_playlist =	g_strcmp0(loop, "Playlist") == 0 ?
						"inf":"no";

		g_object_set(	G_OBJECT(model),
				"loop-file", loop_file,
				"loop-playlist", loop_playlist,
				NULL );
	}
	else if(g_strcmp0(property_name, "Rate") == 0)
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

	return result;
}

static void
update_playback_status(CelluloidMprisPlayer *player)
{
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
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

	celluloid_mpris_module_set_properties(	CELLULOID_MPRIS_MODULE(player),
						"PlaybackStatus",
						g_variant_new_string(state),
						"CanSeek",
						g_variant_new_boolean(can_seek),
						NULL );
}

static void
update_playlist_state(CelluloidMprisPlayer *player)
{
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
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

	celluloid_mpris_module_set_properties(	CELLULOID_MPRIS_MODULE(player),
						"CanGoPrevious",
						g_variant_new_boolean(can_prev),
						"CanGoNext",
						g_variant_new_boolean(can_next),
						NULL );
}

static void
update_speed(CelluloidMprisPlayer *player)
{
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
	gdouble speed = 1.0;

	g_object_get(G_OBJECT(model), "speed", &speed, NULL);

	celluloid_mpris_module_set_properties(	CELLULOID_MPRIS_MODULE(player),
						"Rate",
						g_variant_new_double(speed),
						NULL );
}

static void
update_loop(CelluloidMprisPlayer *player)
{
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
	gchar *loop_file = NULL;
	gchar *loop_playlist = NULL;
	const gchar *loop = NULL;

	g_object_get(	model,
			"loop-file", &loop_file,
			"loop-playlist", &loop_playlist,
			NULL );

	loop =	g_strcmp0(loop_file, "inf") == 0 ? "Track" :
		g_strcmp0(loop_playlist, "inf") == 0 ? "Playlist" :
		"None";

	celluloid_mpris_module_set_properties(	CELLULOID_MPRIS_MODULE(player),
						"LoopStatus",
						g_variant_new_string(loop),
						NULL );

	g_free(loop_file);
	g_free(loop_playlist);
}

static void
update_metadata(CelluloidMprisPlayer *player)
{
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
	GPtrArray *metadata = NULL;
	GVariantBuilder builder;
	gchar *path;
	gchar *uri;
	gdouble duration = 0;
	gint64 playlist_pos = 0;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
	path = celluloid_model_get_current_path(model)?:g_strdup("");
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

	if(playlist_pos >= 0)
	{
		gchar *playlist_pos_str =
			g_strdup_printf("%" G_GINT64_FORMAT, playlist_pos);
		gchar *trackid =
			g_strconcat
			(MPRIS_TRACK_ID_PREFIX, playlist_pos_str, NULL);
		GVariant *object_path =
			g_variant_new_object_path(trackid);

		g_variant_builder_add
			(&builder, "{sv}", "mpris:trackid", object_path);

		g_free(trackid);
		g_free(playlist_pos_str);
	}

	append_metadata_tags(&builder, metadata);

	celluloid_mpris_module_set_properties(	CELLULOID_MPRIS_MODULE(player),
						"Metadata",
						g_variant_new("a{sv}", &builder),
						NULL );

	g_free(path);
	g_free(uri);
}

static void
update_volume(CelluloidMprisPlayer *player)
{
	CelluloidModel *model =	celluloid_controller_get_model
				(player->controller);
	gdouble volume = 0.0;

	g_object_get(G_OBJECT(model), "volume", &volume, NULL);

	celluloid_mpris_module_set_properties
		(	CELLULOID_MPRIS_MODULE(player),
			"Volume",
			g_variant_new_double(volume/100.0),
			NULL );
}

static void
idle_active_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data )
{
	update_playback_status(data);
}

static void
core_idle_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data )
{
	update_playback_status(data);
}

static void
playlist_pos_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data )
{
	update_playlist_state(data);
}

static void
playlist_count_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data )
{
	update_playlist_state(data);
}

static void
speed_handler(	GObject *object,
		GParamSpec *pspec,
		gpointer data )
{
	update_speed(data);
}

static void
loop_handler(	GObject *object,
		GParamSpec *pspec,
		gpointer data )
{
	update_loop(data);
}

static void
metadata_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data )
{
	update_metadata(data);
}

static void
volume_handler(	GObject *object,
		GParamSpec *pspec,
		gpointer data )
{
	update_volume(data);
}

static void
playback_restart_handler(CelluloidModel *model, gpointer data)
{
	GDBusConnection *conn;
	GDBusInterfaceInfo *iface;
	gdouble position;

	position = celluloid_model_get_time_position(model);

	g_object_get(	CELLULOID_MPRIS_MODULE(data),
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

static void
celluloid_mpris_player_class_init(CelluloidMprisPlayerClass *klass)
{
	CelluloidMprisModuleClass *module_class =
		CELLULOID_MPRIS_MODULE_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	module_class->register_interface = register_interface;
	module_class->unregister_interface = unregister_interface;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_pointer
		(	"controller",
			"Controller",
			"The CelluloidApplication to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONTROLLER, pspec);
}

static void
celluloid_mpris_player_init(CelluloidMprisPlayer *player)
{
	const struct
	{
		const gchar *name;
		gboolean readonly;
	}
	properties[] =
	{
		{"PlaybackStatus", TRUE},
		{"LoopStatus", FALSE},
		{"Rate", FALSE},
		{"Metadata", TRUE},
		{"Volume", FALSE},
		{"MinimumRate", TRUE},
		{"MaximumRate", TRUE},
		{"CanGoNext", TRUE},
		{"CanGoPrevious", TRUE},
		{"CanPlay", TRUE},
		{"CanPause", TRUE},
		{"CanSeek", TRUE},
		{"CanControl", TRUE},
		{NULL, FALSE}
	};

	player->controller =
		NULL;
	player->readonly_table =
		g_hash_table_new_full(g_str_hash, g_int_equal, g_free, NULL);
	player->reg_id =
		0;

	for(gint i = 0; properties[i].name; i++)
	{
		g_hash_table_replace
			(	player->readonly_table,
				g_strdup(properties[i].name),
				GINT_TO_POINTER(properties[i].readonly) );
	}
}

CelluloidMprisModule *
celluloid_mpris_player_new(	CelluloidController *controller,
				GDBusConnection *conn )
{
	GType type;
	GDBusInterfaceInfo *iface;

	type = celluloid_mpris_player_get_type();
	iface = celluloid_mpris_org_mpris_media_player2_player_interface_info();

	return CELLULOID_MPRIS_MODULE(g_object_new(	type,
							"controller", controller,
							"conn", conn,
							"iface", iface,
							NULL ));
}
