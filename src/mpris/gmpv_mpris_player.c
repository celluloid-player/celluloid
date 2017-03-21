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

#include "gmpv_application_private.h"
#include "gmpv_main_window.h"
#include "gmpv_mpris.h"
#include "gmpv_mpris_player.h"
#include "gmpv_mpris_gdbus.h"
#include "gmpv_def.h"

static void prop_table_init(gmpv_mpris *inst);
static void emit_prop_changed(	gmpv_mpris *inst,
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
static void update_playback_status(gmpv_mpris *inst);
static void update_playlist_state(gmpv_mpris *inst);


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

static void prop_table_init(gmpv_mpris *inst)
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

	gint i;

	for(i = 0; default_values[i]; i += 2)
	{
		g_hash_table_insert(	inst->player_prop_table,
					default_values[i],
					default_values[i+1] );
	}
}

static void emit_prop_changed(gmpv_mpris *inst, const gmpv_mpris_prop *prop_list)
{
	GDBusInterfaceInfo *iface;

	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();
	gmpv_mpris_emit_prop_changed(inst, iface->name, prop_list);
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
	gmpv_mpris *inst = data;
	GmpvModel *model = inst->gmpv_ctx->model;

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
	gmpv_mpris *inst = data;
	GVariant *value;

	if(g_strcmp0(property_name, "Position") == 0)
	{
		GmpvModel *model;
		gdouble position;

		model = inst->gmpv_ctx->model;
		position = gmpv_model_get_time_position(model);
		value = g_variant_new_int64((gint64)(position*1e6));
	}
	else
	{
		value = g_hash_table_lookup(	inst->player_prop_table,
						property_name );
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
	gmpv_mpris *inst = data;
	GmpvModel *model = inst->gmpv_ctx->model;

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

	g_hash_table_replace(	inst->player_prop_table,
				g_strdup(property_name),
				g_variant_ref(value) );

	return TRUE; /* This function should always succeed */
}

static void update_playback_status(gmpv_mpris *inst)
{
	GmpvModel *model = inst->gmpv_ctx->model;
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

	g_hash_table_replace(	inst->player_prop_table,
				"PlaybackStatus",
				g_variant_ref(state_value) );

	g_hash_table_replace(	inst->player_prop_table,
				"CanSeek",
				g_variant_ref(can_seek_value) );

	prop_list =	(gmpv_mpris_prop[])
			{	{"PlaybackStatus", state_value},
				{"CanSeek", can_seek_value},
				{NULL, NULL} };

	emit_prop_changed(inst, prop_list);
}

static void update_playlist_state(gmpv_mpris *inst)
{
	GmpvModel *model = inst->gmpv_ctx->model;
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

	g_hash_table_replace (	inst->player_prop_table,
				"CanGoPrevious",
				g_variant_ref(can_prev_value) );

	g_hash_table_replace (	inst->player_prop_table,
				"CanGoNext",
				g_variant_ref(can_next_value) );

	prop_list =	(gmpv_mpris_prop[])
			{	{"CanGoPrevious", can_prev_value},
				{"CanGoNext", can_next_value},
				{NULL, NULL} };

	emit_prop_changed(inst, prop_list);
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
	gmpv_mpris *inst = data;
	gdouble speed = 1.0;
	GVariant *value = NULL;
	gmpv_mpris_prop *prop_list = NULL;

	g_object_get(	object,
			"speed", &speed,
			NULL );

	value = g_variant_new_double(speed);

	g_hash_table_replace (	inst->player_prop_table,
				"Rate",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Rate", value}, {NULL, NULL}};

	emit_prop_changed(inst, prop_list);
}

static void metadata_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	gmpv_mpris *inst = data;
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

	g_hash_table_replace (	inst->player_prop_table,
				"Metadata",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Metadata", value}, {NULL, NULL}};
	emit_prop_changed(inst, prop_list);

	g_free(path);
	g_free(uri);
	g_free(playlist_pos_str);
	g_free(trackid);
}

static void volume_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	gmpv_mpris *inst = data;
	gmpv_mpris_prop *prop_list;
	gdouble volume = 0.0;
	GVariant *value;

	g_object_get(object, "volume", &volume, NULL);

	value = g_variant_new_double(volume/100.0);

	g_hash_table_replace(	inst->player_prop_table,
				"Volume",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Volume", value}, {NULL, NULL}};

	emit_prop_changed(inst, prop_list);
}

static void playback_restart_handler(GmpvModel *model, gpointer data)
{
	gmpv_mpris *inst;
	GDBusInterfaceInfo *iface;
	gdouble position;

	inst = data;
	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();
	position = gmpv_model_get_time_position(model);

	g_dbus_connection_emit_signal
		(	inst->session_bus_conn,
			NULL,
			MPRIS_OBJ_ROOT_PATH,
			iface->name,
			"Seeked",
			g_variant_new("(x)", (gint64)(position*1e6)),
			NULL );
}

void gmpv_mpris_player_register(gmpv_mpris *inst)
{
	GmpvModel *model = inst->gmpv_ctx->model;
	GDBusInterfaceInfo *iface;
	GDBusInterfaceVTable vtable;

	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();

	inst->player_prop_table = g_hash_table_new_full
					(	g_str_hash,
						g_str_equal,
						NULL,
						(GDestroyNotify)
						g_variant_unref );

	inst->player_sig_id_list = g_malloc(9*sizeof(gulong));
	inst->player_sig_id_list[0] =	g_signal_connect
					(	model,
						"notify::core-idle",
						G_CALLBACK(core_idle_handler),
						inst );
	inst->player_sig_id_list[1] =	g_signal_connect
					(	model,
						"notify::idle-active",
						G_CALLBACK(idle_active_handler),
						inst );
	inst->player_sig_id_list[2] =	g_signal_connect
					(	model,
						"notify::playlist-pos",
						G_CALLBACK(playlist_pos_handler),
						inst );
	inst->player_sig_id_list[3] =	g_signal_connect
					(	model,
						"notify::playlist-count",
						G_CALLBACK(playlist_count_handler),
						inst );
	inst->player_sig_id_list[4] =	g_signal_connect
					(	model,
						"notify::speed",
						G_CALLBACK(speed_handler),
						inst );
	inst->player_sig_id_list[5] =	g_signal_connect
					(	model,
						"notify::metadata",
						G_CALLBACK(metadata_handler),
						inst );
	inst->player_sig_id_list[6] =	g_signal_connect
					(	model,
						"notify::volume",
						G_CALLBACK(volume_handler),
						inst );
	inst->player_sig_id_list[7] =	g_signal_connect
					(	model,
						"playback-restart",
						G_CALLBACK(playback_restart_handler),
						inst );
	inst->player_sig_id_list[8] = 0;

	prop_table_init(inst);

	vtable.method_call = (GDBusInterfaceMethodCallFunc)method_handler;
	vtable.get_property = (GDBusInterfaceGetPropertyFunc)get_prop_handler;
	vtable.set_property = (GDBusInterfaceSetPropertyFunc)set_prop_handler;

	inst->player_reg_id = g_dbus_connection_register_object
				(	inst->session_bus_conn,
					MPRIS_OBJ_ROOT_PATH,
					iface,
					&vtable,
					inst,
					NULL,
					NULL );
}

void gmpv_mpris_player_unregister(gmpv_mpris *inst)
{
	gulong *current_sig_id = inst->base_sig_id_list;

	if(current_sig_id)
	{
		while(current_sig_id && *current_sig_id > 0)
		{
			GmpvMainWindow *wnd;

			wnd = gmpv_application_get_main_window(inst->gmpv_ctx);
			g_signal_handler_disconnect(wnd, *current_sig_id);

			current_sig_id++;
		}

		g_dbus_connection_unregister_object(	inst->session_bus_conn,
							inst->player_reg_id );

		g_hash_table_remove_all(inst->player_prop_table);
		g_hash_table_unref(inst->player_prop_table);
		g_clear_pointer(&inst->base_sig_id_list, g_free);
	}
}
