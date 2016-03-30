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

#include "mpris_player.h"
#include "mpris_gdbus.h"
#include "mpv_obj.h"
#include "def.h"

static void prop_table_init(mpris *inst);
static void emit_prop_changed(mpris *inst, const mpris_prop_val_pair *prop_list);
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
static void playback_status_update_handler(mpris *inst);
static void playlist_update_handler(mpris *inst);
static void speed_update_handler(mpris *inst);
static void metadata_update_handler(mpris *inst);
static void volume_update_handler(mpris *inst);
static void mpv_init_handler(MainWindow *wnd, gpointer data);
static void mpv_playback_restart_handler(MainWindow *wnd, gpointer data);
static void mpv_prop_change_handler(	MainWindow *wnd,
					gchar *name,
					gpointer data );

static void prop_table_init(mpris *inst)
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

static void emit_prop_changed(mpris *inst, const mpris_prop_val_pair *prop_list)
{
	GDBusInterfaceInfo *iface;

	iface = mpris_org_mpris_media_player2_player_interface_info();
	mpris_emit_prop_changed(inst, iface->name, prop_list);
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
	mpris *inst = data;

	if(g_strcmp0(method_name, "Next") == 0)
	{
		const gchar *cmd[] = {"playlist_next", "weak", NULL};

		mpv_command(inst->gmpv_ctx->mpv->mpv_ctx, cmd);
	}
	else if(g_strcmp0(method_name, "Previous") == 0)
	{
		const gchar *cmd[] = {"playlist_prev", "weak", NULL};

		mpv_command(inst->gmpv_ctx->mpv->mpv_ctx, cmd);
	}
	else if(g_strcmp0(method_name, "Pause") == 0)
	{
		mpv_obj_set_property_flag(inst->gmpv_ctx->mpv, "pause", TRUE);
	}
	else if(g_strcmp0(method_name, "PlayPause") == 0)
	{
		gboolean paused;

		paused =	mpv_obj_get_property_flag
				(inst->gmpv_ctx->mpv, "pause");

		mpv_obj_set_property_flag
			(inst->gmpv_ctx->mpv, "pause", !paused);
	}
	else if(g_strcmp0(method_name, "Stop") == 0)
	{
		const gchar *cmd[] = {"stop", NULL};

		mpv_command(inst->gmpv_ctx->mpv->mpv_ctx, cmd);
	}
	else if(g_strcmp0(method_name, "Play") == 0)
	{
		mpv_obj_set_property_flag(inst->gmpv_ctx->mpv, "pause", FALSE);
	}
	else if(g_strcmp0(method_name, "Seek") == 0)
	{
		gint64 offset_us;
		gdouble position;

		g_variant_get(parameters, "(x)", &offset_us);

		mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );

		position += (gdouble)offset_us/1.0e6;

		mpv_set_property(	inst->gmpv_ctx->mpv->mpv_ctx,
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

			mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&old_index );

			if(index != old_index)
			{
				mpv_set_property(	inst->gmpv_ctx->mpv->mpv_ctx,
							"playlist-pos",
							MPV_FORMAT_INT64,
							&index );
			}
			else
			{
				mpv_playback_restart_handler
					(inst->gmpv_ctx->gui, inst);
			}
		}
	}
	else if(g_strcmp0(method_name, "OpenUri") == 0)
	{
		const gchar *uri;

		g_variant_get(parameters, "(&s)", &uri);
		mpv_obj_load(inst->gmpv_ctx->mpv, uri, FALSE, TRUE);
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
	mpris *inst = data;
	GVariant *value;

	if(g_strcmp0(property_name, "Position") == 0)
	{
		gdouble position;
		gint rc;

		rc = mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
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
	mpris *inst = data;

	if(g_strcmp0(property_name, "Rate") == 0)
	{
		gdouble rate = g_variant_get_double(value);

		mpv_set_property(	inst->gmpv_ctx->mpv->mpv_ctx,
					"speed",
					MPV_FORMAT_DOUBLE,
					&rate );
	}
	else if(g_strcmp0(property_name, "Volume") == 0)
	{
		gdouble volume = 100*g_variant_get_double(value);

		mpv_set_property(	inst->gmpv_ctx->mpv->mpv_ctx,
					"volume",
					MPV_FORMAT_DOUBLE,
					&volume );
	}

	g_hash_table_replace(	inst->player_prop_table,
				g_strdup(property_name),
				g_variant_ref(value) );

	return TRUE; /* This function should always succeed */
}

static void playback_status_update_handler(mpris *inst)
{
	const gchar *state;
	mpris_prop_val_pair *prop_list;
	GVariant *state_value;
	GVariant *can_seek_value;
	gint idle;
	gint core_idle;
	gboolean can_seek;

	mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				"idle",
				MPV_FORMAT_FLAG,
				&idle );

	mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				"core-idle",
				MPV_FORMAT_FLAG,
				&core_idle );

	if(!core_idle && !idle)
	{
		state = "Playing";
		can_seek = TRUE;

		mpv_playback_restart_handler(inst->gmpv_ctx->gui, inst);
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

	prop_list =	(mpris_prop_val_pair[])
			{	{"PlaybackStatus", state_value},
				{"CanSeek", can_seek_value},
				{NULL, NULL} };

	emit_prop_changed(inst, prop_list);
}

static void playlist_update_handler(mpris *inst)
{
	mpris_prop_val_pair *prop_list;
	GVariant *can_prev_value;
	GVariant *can_next_value;
	gboolean can_prev;
	gboolean can_next;
	gint64 playlist_count;
	gint64 playlist_pos;
	gint rc = 0;

	rc |= mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				"playlist-count",
				MPV_FORMAT_INT64,
				&playlist_count );

	rc |= mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
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

	prop_list =	(mpris_prop_val_pair[])
			{	{"CanGoPrevious", can_prev_value},
				{"CanGoNext", can_next_value},
				{NULL, NULL} };

	emit_prop_changed(inst, prop_list);
}

static void speed_update_handler(mpris *inst)
{
	gdouble speed;
	GVariant *value;
	mpris_prop_val_pair *prop_list;

	mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				"speed",
				MPV_FORMAT_DOUBLE,
				&speed );

	value = g_variant_new_double(speed);

	g_hash_table_replace (	inst->player_prop_table,
				"Rate",
				g_variant_ref(value) );

	prop_list =	(mpris_prop_val_pair[])
			{{"Rate", value}, {NULL, NULL}};

	emit_prop_changed(inst, prop_list);
}

static void metadata_update_handler(mpris *inst)
{
	const struct
	{
		const gchar *mpv_name;
		const gchar *xesam_name;
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

	mpris_prop_val_pair *prop_list;
	GVariantBuilder builder;
	GVariant *value;
	gchar *uri;
	gchar *playlist_pos_str;
	gchar *trackid;
	gdouble duration;
	gint64 playlist_pos;
	gint rc;
	gint i;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	uri = mpv_get_property_string(inst->gmpv_ctx->mpv->mpv_ctx, "path");

	if(uri)
	{
		g_variant_builder_add(	&builder,
					"{sv}",
					"xesam:url",
					g_variant_new_string(uri) );

		mpv_free(uri);
	}

	rc = mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				"duration",
				MPV_FORMAT_DOUBLE,
				&duration );

	g_variant_builder_add(	&builder,
				"{sv}",
				"mpris:length",
				g_variant_new_int64
				((gint64)((rc >= 0)*duration*1e6)) );

	rc = mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
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

	for(i = 0; tag_map[i].mpv_name; i++)
	{
		GVariantBuilder tag_builder;
		GVariant *tag_value;
		gchar *mpv_prop;
		gchar *value_buf;

		mpv_prop = g_strconcat(	"metadata/by-key/",
					tag_map[i].mpv_name,
					NULL );

		value_buf = mpv_get_property_string
				(inst->gmpv_ctx->mpv->mpv_ctx, mpv_prop);

		if(value_buf)
		{
			tag_value = g_variant_new_string(value_buf?:"");

			if(tag_map[i].is_array)
			{
				g_variant_builder_init
					(&tag_builder, G_VARIANT_TYPE("as"));

				g_variant_builder_add_value
					(&tag_builder, tag_value);
			}

			g_variant_builder_add
				(	&builder,
					"{sv}",
					tag_map[i].xesam_name,
					tag_map[i].is_array?
					g_variant_new("as", &tag_builder):
					tag_value );

			mpv_free(value_buf);
		}

		g_free(mpv_prop);
	}

	value = g_variant_new("a{sv}", &builder);

	g_hash_table_replace (	inst->player_prop_table,
				"Metadata",
				g_variant_ref(value) );

	prop_list =	(mpris_prop_val_pair[])
			{{"Metadata", value}, {NULL, NULL}};

	emit_prop_changed(inst, prop_list);
	mpv_playback_restart_handler(inst->gmpv_ctx->gui, inst);

	g_free(playlist_pos_str);
	g_free(trackid);
}

static void volume_update_handler(mpris *inst)
{
	mpris_prop_val_pair *prop_list;
	gdouble volume;
	GVariant *value;

	mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				"volume",
				MPV_FORMAT_DOUBLE,
				&volume );

	value = g_variant_new_double(volume/100.0);

	g_hash_table_replace(	inst->player_prop_table,
				"Volume",
				g_variant_ref(value) );

	prop_list =	(mpris_prop_val_pair[])
			{{"Volume", value}, {NULL, NULL}};

	emit_prop_changed(inst, prop_list);
}

static void mpv_init_handler(MainWindow *wnd, gpointer data)
{
	mpris *inst = data;

	mpv_observe_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				0,
				"idle",
				MPV_FORMAT_FLAG );

	mpv_observe_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				0,
				"core-idle",
				MPV_FORMAT_FLAG );

	mpv_observe_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				0,
				"speed",
				MPV_FORMAT_DOUBLE );

	mpv_observe_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				0,
				"metadata",
				MPV_FORMAT_NODE );

	mpv_observe_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				0,
				"volume",
				MPV_FORMAT_DOUBLE );

	mpv_observe_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				0,
				"playlist-pos",
				MPV_FORMAT_INT64 );

	mpv_observe_property(	inst->gmpv_ctx->mpv->mpv_ctx,
				0,
				"playlist-count",
				MPV_FORMAT_INT64 );
}

static void mpv_playback_restart_handler(MainWindow *wnd, gpointer data)
{
	mpris *inst = data;
	GDBusInterfaceInfo *iface;
	gdouble position;

	if(inst->pending_seek > 0)
	{
		position = inst->pending_seek;
		inst->pending_seek = -1;

		mpv_set_property(	inst->gmpv_ctx->mpv->mpv_ctx,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );
	}
	else
	{
		mpv_get_property(	inst->gmpv_ctx->mpv->mpv_ctx,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&position );
	}

	iface = mpris_org_mpris_media_player2_player_interface_info();

	g_dbus_connection_emit_signal
		(	inst->session_bus_conn,
			NULL,
			MPRIS_OBJ_ROOT_PATH,
			iface->name,
			"Seeked",
			g_variant_new("(x)", (gint64)(position*1e6)),
			NULL );
}

static void mpv_prop_change_handler(	MainWindow *wnd,
					gchar *name,
					gpointer data )
{
	mpris *inst = data;

	if(g_strcmp0(name, "core-idle") == 0
	|| g_strcmp0(name, "idle") == 0)
	{
		playback_status_update_handler(inst);
	}
	else if(g_strcmp0(name, "playlist-pos") == 0
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

void mpris_player_register(mpris *inst)
{
	GDBusInterfaceInfo *iface;
	GDBusInterfaceVTable vtable;

	iface = mpris_org_mpris_media_player2_player_interface_info();

	inst->player_prop_table = g_hash_table_new_full
					(	g_str_hash,
						g_str_equal,
						NULL,
						(GDestroyNotify)
						g_variant_unref );

	inst->player_sig_id_list = g_malloc(4*sizeof(gulong));

	inst->player_sig_id_list[0]
		= g_signal_connect(	inst->gmpv_ctx->mpv,
					"mpv-init",
					G_CALLBACK(mpv_init_handler),
					inst );

	inst->player_sig_id_list[1]
		= g_signal_connect(	inst->gmpv_ctx->mpv,
					"mpv-playback-restart",
					G_CALLBACK(mpv_playback_restart_handler),
					inst );

	inst->player_sig_id_list[2]
		= g_signal_connect(	inst->gmpv_ctx->mpv,
					"mpv-prop-change",
					G_CALLBACK(mpv_prop_change_handler),
					inst );

	inst->player_sig_id_list[3] = 0;

	mpv_init_handler(inst->gmpv_ctx->gui, inst);
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

void mpris_player_unregister(mpris *inst)
{
	gulong *current_sig_id = inst->player_sig_id_list;

	if(current_sig_id)
	{
		while(current_sig_id && *current_sig_id > 0)
		{
			g_signal_handler_disconnect(	inst->gmpv_ctx->gui,
							*current_sig_id );

			current_sig_id++;
		}

		g_dbus_connection_unregister_object(	inst->session_bus_conn,
							inst->player_reg_id );

		g_hash_table_remove_all(inst->player_prop_table);
		g_hash_table_unref(inst->player_prop_table);
		g_clear_pointer(&inst->player_sig_id_list, g_free);
	}
}
