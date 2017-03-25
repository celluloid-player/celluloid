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
	PROP_CONN,
	N_PROPERTIES
};


struct _GmpvMprisPlayer
{
	GObject parent;
	GmpvApplication *app;
	GDBusConnection *conn;
	guint reg_id;
	GHashTable *prop_table;
	gulong *sig_id_list;
};

struct _GmpvMprisPlayerClass
{
	GObject parent_class;
};

static void register_interface(GmpvMprisModule *module);
static void unregister_interface(GmpvMprisModule *module);

static void constructed(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void prop_table_init(GHashTable *table);
static void emit_prop_changed(	GmpvMprisPlayer *player,
				const gmpv_mpris_prop *prop_list );
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
	GDBusInterfaceInfo *iface;
	GDBusInterfaceVTable vtable;

	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();

	player->prop_table =	g_hash_table_new_full
				(	g_str_hash,
					g_str_equal,
					NULL,
					(GDestroyNotify)
					g_variant_unref );

	player->sig_id_list = g_malloc(9*sizeof(gulong));
	player->sig_id_list[0] =	g_signal_connect
					(	model,
						"notify::core-idle",
						G_CALLBACK(core_idle_handler),
						player );
	player->sig_id_list[1] =	g_signal_connect
					(	model,
						"notify::idle-active",
						G_CALLBACK(idle_active_handler),
						player );
	player->sig_id_list[2] =	g_signal_connect
					(	model,
						"notify::playlist-pos",
						G_CALLBACK(playlist_pos_handler),
						player );
	player->sig_id_list[3] =	g_signal_connect
					(	model,
						"notify::playlist-count",
						G_CALLBACK(playlist_count_handler),
						player );
	player->sig_id_list[4] =	g_signal_connect
					(	model,
						"notify::speed",
						G_CALLBACK(speed_handler),
						player );
	player->sig_id_list[5] =	g_signal_connect
					(	model,
						"notify::metadata",
						G_CALLBACK(metadata_handler),
						player );
	player->sig_id_list[6] =	g_signal_connect
					(	model,
						"notify::volume",
						G_CALLBACK(volume_handler),
						player );
	player->sig_id_list[7] =	g_signal_connect
					(	model,
						"playback-restart",
						G_CALLBACK(playback_restart_handler),
						player );
	player->sig_id_list[8] = 0;

	prop_table_init(player->prop_table);

	vtable.method_call = (GDBusInterfaceMethodCallFunc)method_handler;
	vtable.get_property = (GDBusInterfaceGetPropertyFunc)get_prop_handler;
	vtable.set_property = (GDBusInterfaceSetPropertyFunc)set_prop_handler;

	player->reg_id = g_dbus_connection_register_object
				(	player->conn,
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
	gulong *current_sig_id = player->sig_id_list;

	if(current_sig_id)
	{
		while(current_sig_id && *current_sig_id > 0)
		{
			g_signal_handler_disconnect(	player->app->model,
							*current_sig_id );

			current_sig_id++;
		}

		g_dbus_connection_unregister_object(	player->conn,
							player->reg_id );

		g_hash_table_remove_all(player->prop_table);
		g_hash_table_unref(player->prop_table);
		g_clear_pointer(&player->sig_id_list, g_free);
	}
}

static void constructed(GObject *object)
{
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

		case PROP_CONN:
		self->conn = g_value_get_pointer(value);
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

		case PROP_CONN:
		g_value_set_pointer(value, self->conn);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void prop_table_init(GHashTable *table)
{
	/* Position is retrieved from mpv on-demand */
	const gpointer default_values[]
		= {	"PlaybackStatus", g_variant_new_string("Stopped"),
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
			NULL };

	for(gint i = 0; default_values[i]; i += 2)
	{
		g_hash_table_insert(	table,
					default_values[i],
					default_values[i+1] );
	}
}

static void emit_prop_changed(GmpvMprisPlayer *player, const gmpv_mpris_prop *prop_list)
{
	GDBusInterfaceInfo *iface;

	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();
	gmpv_mpris_emit_prop_changed(player->conn, iface->name, prop_list);
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

		g_object_get(	G_OBJECT(model),
				"pause", &pause,
				NULL );

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
		value = g_hash_table_lookup(player->prop_table, property_name);
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

	g_hash_table_replace(	player->prop_table,
				g_strdup(property_name),
				g_variant_ref(value) );

	return TRUE; /* This function should always succeed */
}

static void update_playback_status(GmpvMprisPlayer *player)
{
	GmpvModel *model = player->app->model;
	const gchar *state;
	gmpv_mpris_prop *prop_list;
	GVariant *state_value;
	GVariant *can_seek_value;
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

	state_value = g_variant_new_string(state);
	can_seek_value = g_variant_new_boolean(can_seek);

	g_hash_table_replace(	player->prop_table,
				"PlaybackStatus",
				g_variant_ref(state_value) );

	g_hash_table_replace(	player->prop_table,
				"CanSeek",
				g_variant_ref(can_seek_value) );

	prop_list =	(gmpv_mpris_prop[])
			{	{"PlaybackStatus", state_value},
				{"CanSeek", can_seek_value},
				{NULL, NULL} };

	emit_prop_changed(player, prop_list);
}

static void update_playlist_state(GmpvMprisPlayer *player)
{
	GmpvModel *model = player->app->model;
	gmpv_mpris_prop *prop_list;
	GVariant *can_prev_value;
	GVariant *can_next_value;
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
	can_prev_value = g_variant_new_boolean(can_prev);
	can_next_value = g_variant_new_boolean(can_next);

	g_hash_table_replace (	player->prop_table,
				"CanGoPrevious",
				g_variant_ref(can_prev_value) );
	g_hash_table_replace (	player->prop_table,
				"CanGoNext",
				g_variant_ref(can_next_value) );

	prop_list =	(gmpv_mpris_prop[])
			{	{"CanGoPrevious", can_prev_value},
				{"CanGoNext", can_next_value},
				{NULL, NULL} };

	emit_prop_changed(player, prop_list);
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
	GmpvMprisPlayer *player = data;
	gdouble speed = 1.0;
	GVariant *value = NULL;
	gmpv_mpris_prop *prop_list = NULL;

	g_object_get(	object,
			"speed", &speed,
			NULL );

	value = g_variant_new_double(speed);

	g_hash_table_replace (	player->prop_table,
				"Rate",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Rate", value}, {NULL, NULL}};

	emit_prop_changed(player, prop_list);
}

static void metadata_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvMprisPlayer *player = data;
	GmpvModel *model = GMPV_MODEL(object);
	gmpv_mpris_prop *prop_list;
	GSList *metadata = NULL;
	GVariantBuilder builder;
	GVariant *value;
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

	value = g_variant_new("a{sv}", &builder);

	g_hash_table_replace (	player->prop_table,
				"Metadata",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Metadata", value}, {NULL, NULL}};
	emit_prop_changed(player, prop_list);

	g_free(path);
	g_free(uri);
	g_free(playlist_pos_str);
	g_free(trackid);
}

static void volume_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvMprisPlayer *player = data;
	gmpv_mpris_prop *prop_list;
	gdouble volume = 0.0;
	GVariant *value;

	g_object_get(object, "volume", &volume, NULL);

	value = g_variant_new_double(volume/100.0);

	g_hash_table_replace(	player->prop_table,
				"Volume",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Volume", value}, {NULL, NULL}};

	emit_prop_changed(player, prop_list);
}

static void playback_restart_handler(GmpvModel *model, gpointer data)
{
	GmpvMprisPlayer *player;
	GDBusInterfaceInfo *iface;
	gdouble position;

	player = data;
	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();
	position = gmpv_model_get_time_position(model);

	g_dbus_connection_emit_signal
		(	player->conn,
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
	object_class->constructed = constructed;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_pointer
		(	"app",
			"Application",
			"The GmpvApplication to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_APP, pspec);

	pspec = g_param_spec_pointer
		(	"conn",
			"Connection",
			"Connection to the session bus",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONN, pspec);
}

static void gmpv_mpris_player_init(GmpvMprisPlayer *player)
{
	player->app = NULL;
	player->conn = NULL;
	player->reg_id = 0;
	player->prop_table = g_hash_table_new(g_str_hash, g_str_equal);
	player->sig_id_list = 0;
}

GmpvMprisPlayer *gmpv_mpris_player_new(	GmpvApplication *app,
					GDBusConnection *conn )
{
	return GMPV_MPRIS_PLAYER(g_object_new(	gmpv_mpris_player_get_type(),
						"app", app,
						"conn", conn,
						NULL ));
}

