/*
 * Copyright (c) 2015-2016 gnome-mpv
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

#include <string.h>

#include "gmpv_mpris_player.h"
#include "gmpv_mpris_gdbus.h"
#include "gmpv_mpv_obj.h"
#include "gmpv_def.h"

static void prop_table_init(gmpv_mpris *inst);
static void emit_prop_changed(	gmpv_mpris *inst,
				const gmpv_mpris_prop *prop_list );
static void append_metadata_tags(GVariantBuilder *builder, mpv_node_list *list);
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
static void playback_status_update_handler(gmpv_mpris *inst);
static void playlist_update_handler(gmpv_mpris *inst);
static void speed_update_handler(gmpv_mpris *inst);
static void metadata_update_handler(gmpv_mpris *inst);
static void volume_update_handler(gmpv_mpris *inst);
static void mpv_init_handler(GmpvMainWindow *wnd, gpointer data);
static void mpv_playback_restart_handler(GmpvMainWindow *wnd, gpointer data);
static void mpv_prop_change_handler(	GmpvMainWindow *wnd,
					gchar *name,
					gpointer data );

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

static void append_metadata_tags(GVariantBuilder *builder, mpv_node_list *list)
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

	g_assert(list);

	for(gint i = 0; i < list->num; i++)
	{
		GVariantBuilder tag_builder;
		mpv_node mpv_value = list->values[i];
		const gchar *mpv_key = list->keys[i];
		const gchar *tag_name;
		GVariant *tag_value;
		gboolean is_array = TRUE;
		gboolean valid = TRUE;
		gint j = -1;

		g_assert(mpv_key);

		while(	tag_map[++j].mpv_name &&
			g_ascii_strcasecmp(mpv_key, tag_map[j].mpv_name) != 0 );
		tag_name = tag_map[j].mpv_name?tag_map[j].tag_name:mpv_key;
		is_array = tag_map[j].mpv_name?tag_map[j].is_array:FALSE;

		if(!is_array && mpv_value.format == MPV_FORMAT_STRING)
		{
			tag_value = g_variant_new_string(mpv_value.u.string);
		}
		else if(is_array && mpv_value.format == MPV_FORMAT_STRING)
		{
			GVariant *elem_value = 	g_variant_new_string
						(mpv_value.u.string);

			g_variant_builder_init
				(&tag_builder, G_VARIANT_TYPE("as"));
			g_variant_builder_add_value
				(&tag_builder, elem_value);

			tag_value = g_variant_new("as", &tag_builder);
		}
		else
		{
			valid = FALSE;
		}

		if(valid)
		{
			g_debug(	"Adding metadata tag %s with type %d",
					tag_name, mpv_value.format);

			g_variant_builder_add
				(builder, "{sv}", tag_name, tag_value);
		}
		else
		{
			g_warning(	"Ignoring metadata entry \"%s\" "
					"with unsupported format %d",
					tag_name,
					mpv_value.format );
		}
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
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);

	if(g_strcmp0(method_name, "Next") == 0)
	{
		const gchar *cmd[] = {"playlist_next", "weak", NULL};

		gmpv_mpv_obj_command(mpv, cmd);
	}
	else if(g_strcmp0(method_name, "Previous") == 0)
	{
		const gchar *cmd[] = {"playlist_prev", "weak", NULL};

		gmpv_mpv_obj_command(mpv, cmd);
	}
	else if(g_strcmp0(method_name, "Pause") == 0)
	{
		gmpv_mpv_obj_set_property_flag(mpv, "pause", TRUE);
	}
	else if(g_strcmp0(method_name, "PlayPause") == 0)
	{
		gboolean paused;

		paused = gmpv_mpv_obj_get_property_flag(mpv, "pause");

		gmpv_mpv_obj_set_property_flag(mpv, "pause", !paused);
	}
	else if(g_strcmp0(method_name, "Stop") == 0)
	{
		const gchar *cmd[] = {"stop", NULL};

		gmpv_mpv_obj_command(mpv, cmd);
	}
	else if(g_strcmp0(method_name, "Play") == 0)
	{
		gmpv_mpv_obj_set_property_flag(mpv, "pause", FALSE);
	}
	else if(g_strcmp0(method_name, "Seek") == 0)
	{
		gint64 offset_us;
		gdouble position;

		g_variant_get(parameters, "(x)", &offset_us);

		gmpv_mpv_obj_get_property(	mpv,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );

		position += (gdouble)offset_us/1.0e6;

		gmpv_mpv_obj_set_property(	mpv,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );

		g_dbus_connection_emit_signal
			(	inst->session_bus_conn,
				NULL,
				MPRIS_OBJ_ROOT_PATH,
				interface_name,
				"Seeked",
				g_variant_new("(x)", (gint64)(position*1e6)),
				NULL );
	}
	else if(g_strcmp0(method_name, "SetPosition") == 0)
	{
		const gchar *prefix = MPRIS_TRACK_ID_PREFIX;
		const gsize prefix_len = strlen(prefix);
		gint64 index;
		gint64 old_index;
		gint64 time_us;
		const gchar *track_id;

		g_variant_get(parameters, "(&ox)", &track_id, &time_us);

		if(strncmp(track_id, prefix, prefix_len) == 0)
		{
			inst->pending_seek = (gdouble)time_us/1.0e6;
			index = g_ascii_strtoll(track_id+prefix_len, NULL, 0);

			gmpv_mpv_obj_get_property(	mpv,
							"playlist-pos",
							MPV_FORMAT_INT64,
							&old_index );

			if(index != old_index)
			{
				gmpv_mpv_obj_set_property(	mpv,
								"playlist-pos",
								MPV_FORMAT_INT64,
								&index );
			}
			else
			{
				GmpvMainWindow *wnd;

				wnd =	gmpv_application_get_main_window
					(inst->gmpv_ctx);
				mpv_playback_restart_handler(wnd, inst);
			}
		}
	}
	else if(g_strcmp0(method_name, "OpenUri") == 0)
	{
		const gchar *uri;

		g_variant_get(parameters, "(&s)", &uri);
		gmpv_mpv_obj_load(mpv, uri, FALSE, TRUE);
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
		GmpvMpvObj *mpv;
		mpv_handle *mpv_ctx;
		gdouble position;
		gint rc;

		mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
		mpv_ctx = gmpv_mpv_obj_get_mpv_handle(mpv);
		rc = mpv_get_property(	mpv_ctx,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );
		value = g_variant_new_int64((gint64)((rc >= 0)*position*1e6));
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
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);

	if(g_strcmp0(property_name, "Rate") == 0)
	{
		gdouble rate = g_variant_get_double(value);

		gmpv_mpv_obj_set_property(	mpv,
					"speed",
					MPV_FORMAT_DOUBLE,
					&rate );
	}
	else if(g_strcmp0(property_name, "Volume") == 0)
	{
		gdouble volume = 100*g_variant_get_double(value);

		gmpv_mpv_obj_set_property(	mpv,
					"volume",
					MPV_FORMAT_DOUBLE,
					&volume );
	}

	g_hash_table_replace(	inst->player_prop_table,
				g_strdup(property_name),
				g_variant_ref(value) );

	return TRUE; /* This function should always succeed */
}

static void playback_status_update_handler(gmpv_mpris *inst)
{
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
	const gchar *state;
	gmpv_mpris_prop *prop_list;
	GVariant *state_value;
	GVariant *can_seek_value;
	gint idle;
	gint core_idle;
	gboolean can_seek;

	gmpv_mpv_obj_get_property(mpv, "idle", MPV_FORMAT_FLAG, &idle);
	gmpv_mpv_obj_get_property(mpv, "core-idle", MPV_FORMAT_FLAG, &core_idle);

	if(!core_idle && !idle)
	{
		GmpvMainWindow *wnd =	gmpv_application_get_main_window
					(inst->gmpv_ctx);

		state = "Playing";
		can_seek = TRUE;

		mpv_playback_restart_handler(wnd, inst);
	}
	else if(core_idle && idle)
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

static void playlist_update_handler(gmpv_mpris *inst)
{
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
	gmpv_mpris_prop *prop_list;
	GVariant *can_prev_value;
	GVariant *can_next_value;
	gboolean can_prev;
	gboolean can_next;
	gint64 playlist_count;
	gint64 playlist_pos;
	gint rc = 0;

	rc |= gmpv_mpv_obj_get_property(	mpv,
						"playlist-count",
						MPV_FORMAT_INT64,
						&playlist_count );
	rc |= gmpv_mpv_obj_get_property(	mpv,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );

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

static void speed_update_handler(gmpv_mpris *inst)
{
	gdouble speed;
	GVariant *value;
	gmpv_mpris_prop *prop_list;

	gmpv_mpv_obj_get_property
		(	gmpv_application_get_mpv_obj(inst->gmpv_ctx),
			"speed",
			MPV_FORMAT_DOUBLE,
			&speed );

	value = g_variant_new_double(speed);

	g_hash_table_replace (	inst->player_prop_table,
				"Rate",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Rate", value}, {NULL, NULL}};

	emit_prop_changed(inst, prop_list);
}

static void metadata_update_handler(gmpv_mpris *inst)
{
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
	GmpvMainWindow *wnd = gmpv_application_get_main_window(inst->gmpv_ctx);
	gmpv_mpris_prop *prop_list;
	mpv_node metadata;
	GVariantBuilder builder;
	GVariant *value;
	gchar *uri;
	gchar *playlist_pos_str;
	gchar *trackid;
	gdouble duration;
	gint64 playlist_pos;
	gint rc;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	uri = gmpv_mpv_obj_get_property_string(mpv, "path");

	if(uri)
	{
		g_variant_builder_add(	&builder,
					"{sv}",
					"xesam:url",
					g_variant_new_string(uri) );

		mpv_free(uri);
	}

	rc = gmpv_mpv_obj_get_property(	mpv,
					"duration",
					MPV_FORMAT_DOUBLE,
					&duration );
	g_variant_builder_add(	&builder,
				"{sv}",
				"mpris:length",
				g_variant_new_int64
				((gint64)((rc >= 0)*duration*1e6)) );

	rc = gmpv_mpv_obj_get_property(	mpv,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&playlist_pos );
	playlist_pos_str = g_strdup_printf(	"%" G_GINT64_FORMAT,
						(rc >= 0)*playlist_pos );

	trackid = g_strconcat(	MPRIS_TRACK_ID_PREFIX,
				playlist_pos_str,
				NULL );
	g_variant_builder_add(	&builder,
				"{sv}",
				"mpris:trackid",
				g_variant_new_object_path(trackid) );

	rc = gmpv_mpv_obj_get_property(	mpv,
					"metadata",
					MPV_FORMAT_NODE,
					&metadata );

	if(rc >= 0)
	{
		append_metadata_tags(&builder, metadata.u.list);
	}

	value = g_variant_new("a{sv}", &builder);

	g_hash_table_replace (	inst->player_prop_table,
				"Metadata",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Metadata", value}, {NULL, NULL}};
	emit_prop_changed(inst, prop_list);

	mpv_playback_restart_handler(wnd, inst);

	mpv_free_node_contents(&metadata);
	g_free(playlist_pos_str);
	g_free(trackid);
}

static void volume_update_handler(gmpv_mpris *inst)
{
	gmpv_mpris_prop *prop_list;
	gdouble volume;
	GVariant *value;

	gmpv_mpv_obj_get_property
		(	gmpv_application_get_mpv_obj(inst->gmpv_ctx),
			"volume",
			MPV_FORMAT_DOUBLE,
			&volume );

	value = g_variant_new_double(volume/100.0);

	g_hash_table_replace(	inst->player_prop_table,
				"Volume",
				g_variant_ref(value) );

	prop_list =	(gmpv_mpris_prop[])
			{{"Volume", value}, {NULL, NULL}};

	emit_prop_changed(inst, prop_list);
}

static void mpv_init_handler(GmpvMainWindow *wnd, gpointer data)
{
	gmpv_mpris *inst = data;
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
	mpv_handle *mpv_ctx = gmpv_mpv_obj_get_mpv_handle(mpv);

	mpv_observe_property(mpv_ctx, 0, "idle", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv_ctx, 0, "core-idle", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv_ctx, 0, "speed", MPV_FORMAT_DOUBLE);
	mpv_observe_property(mpv_ctx, 0, "metadata", MPV_FORMAT_NODE);
	mpv_observe_property(mpv_ctx, 0, "volume", MPV_FORMAT_DOUBLE);
	mpv_observe_property(mpv_ctx, 0, "playlist-pos", MPV_FORMAT_INT64);
	mpv_observe_property(mpv_ctx, 0, "playlist-count", MPV_FORMAT_INT64);
}

static void mpv_playback_restart_handler(GmpvMainWindow *wnd, gpointer data)
{
	gmpv_mpris *inst = data;
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
	GDBusInterfaceInfo *iface;
	gdouble position;

	if(inst->pending_seek > 0)
	{
		position = inst->pending_seek;
		inst->pending_seek = -1;

		gmpv_mpv_obj_set_property(	mpv,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );
	}
	else
	{
		gmpv_mpv_obj_get_property(	mpv,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );
	}

	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();

	g_dbus_connection_emit_signal
		(	inst->session_bus_conn,
			NULL,
			MPRIS_OBJ_ROOT_PATH,
			iface->name,
			"Seeked",
			g_variant_new("(x)", (gint64)(position*1e6)),
			NULL );
}

static void mpv_prop_change_handler(	GmpvMainWindow *wnd,
					gchar *name,
					gpointer data )
{
	gmpv_mpris *inst = data;
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
	GmpvMpvObjState state;

	gmpv_mpv_obj_get_state(mpv, &state);

	if(g_strcmp0(name, "core-idle") == 0
	|| g_strcmp0(name, "idle") == 0)
	{
		playback_status_update_handler(inst);
	}
	else if(state.loaded)
	{
		if(g_strcmp0(name, "playlist-pos") == 0
		|| g_strcmp0(name, "playlist-count") == 0)
		{
			playlist_update_handler(inst);
		}
		else if(g_strcmp0(name, "speed") == 0)
		{
			speed_update_handler(inst);
		}
		else if(g_strcmp0(name, "metadata") == 0)
		{
			metadata_update_handler(inst);
		}
		else if(g_strcmp0(name, "volume") == 0)
		{
			volume_update_handler(inst);
		}
	}
}

void gmpv_mpris_player_register(gmpv_mpris *inst)
{
	GmpvMpvObj *mpv = gmpv_application_get_mpv_obj(inst->gmpv_ctx);
	GDBusInterfaceInfo *iface;
	GDBusInterfaceVTable vtable;

	iface = gmpv_mpris_org_mpris_media_player2_player_interface_info();

	inst->player_prop_table = g_hash_table_new_full
					(	g_str_hash,
						g_str_equal,
						NULL,
						(GDestroyNotify)
						g_variant_unref );

	inst->player_sig_id_list = g_malloc(4*sizeof(gulong));
	inst->player_sig_id_list[0]
		= g_signal_connect(	mpv,
					"mpv-init",
					G_CALLBACK(mpv_init_handler),
					inst );
	inst->player_sig_id_list[1]
		= g_signal_connect(	mpv,
					"mpv-playback-restart",
					G_CALLBACK(mpv_playback_restart_handler),
					inst );
	inst->player_sig_id_list[2]
		= g_signal_connect(	mpv,
					"mpv-prop-change",
					G_CALLBACK(mpv_prop_change_handler),
					inst );
	inst->player_sig_id_list[3] = 0;

	mpv_init_handler(gmpv_application_get_main_window(inst->gmpv_ctx), inst);
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
	gulong *current_sig_id = inst->player_sig_id_list;

	if(current_sig_id)
	{
		GmpvMainWindow *wnd =	gmpv_application_get_main_window
					(inst->gmpv_ctx);

		while(current_sig_id && *current_sig_id > 0)
		{
			g_signal_handler_disconnect(wnd, *current_sig_id);

			current_sig_id++;
		}

		g_dbus_connection_unregister_object(	inst->session_bus_conn,
							inst->player_reg_id );

		g_hash_table_remove_all(inst->player_prop_table);
		g_hash_table_unref(inst->player_prop_table);
		g_clear_pointer(&inst->player_sig_id_list, g_free);
	}
}
