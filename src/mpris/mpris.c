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

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "mpris.h"
#include "mpris_base.h"
#include "mpris_player.h"
#include "mpris_gdbus.h" /* auto-generated */
#include "def.h"

static void bus_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data );
static void name_lost_handler(	GDBusConnection *connection,
				const gchar *name,
				gpointer data );
static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data );
static void unregister(mpris *inst);

static void bus_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data )
{
	mpris *inst = data;

	inst->session_bus_conn = connection;

	mpris_base_register(inst);
	mpris_player_register(inst);
}

static void name_lost_handler(	GDBusConnection *connection,
				const gchar *name,
				gpointer data )
{
	unregister(data);
}

static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	mpris *inst = data;

	g_signal_handler_disconnect(	inst->gmpv_ctx->gui,
					inst->shutdown_sig_id );

	unregister(inst);
	g_bus_unown_name(inst->name_id);
	g_free(inst);

	return FALSE;
}

static void unregister(mpris *inst)
{
	if(inst->base_reg_id > 0)
	{
		mpris_base_unregister(inst);
	}

	if(inst->player_reg_id > 0)
	{
		mpris_player_unregister(inst);
	}
}

void mpris_emit_prop_changed(mpris *inst, const mpris_prop_val_pair *prop_list)
{
	const mpris_prop_val_pair *current;
	GDBusInterfaceInfo *iface;
	GVariantBuilder builder;
	GVariant *sig_args;

	current = prop_list;
	iface = mpris_org_mpris_media_player2_interface_info();

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	while(current && current->name != NULL)
	{
		g_variant_builder_add(	&builder,
					"{sv}",
					current->name,
					current->value );

		current++;
	}

	sig_args = g_variant_new(	"(sa{sv}as)",
					iface->name,
					&builder,
					NULL );

	g_dbus_connection_emit_signal
		(	inst->session_bus_conn,
			NULL,
			MPRIS_OBJ_ROOT_PATH,
			"org.freedesktop.DBus.Properties",
			"PropertiesChanged",
			sig_args,
			NULL );
}

GVariant *mpris_build_g_variant_string_array(const gchar** list)
{
	GVariantBuilder builder;
	gint i;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

	for(i = 0; list[i]; i++)
	{
		g_variant_builder_add(&builder, "s", list[i]);
	}

	return g_variant_new("as", &builder);
}

void mpris_init(Application *gmpv_ctx)
{
	mpris *inst = g_malloc(sizeof(mpris));

	inst->gmpv_ctx = gmpv_ctx;
	inst->name_id = 0;
	inst->base_reg_id = 0;
	inst->player_reg_id = 0;
	inst->shutdown_sig_id = 0;
	inst->base_sig_id_list = NULL;
	inst->player_sig_id_list = NULL;
	inst->pending_seek = -1;
	inst->base_prop_table = NULL;
	inst->player_prop_table = NULL;
	inst->session_bus_conn = NULL;

	inst->shutdown_sig_id
		= g_signal_connect(	inst->gmpv_ctx->gui,
					"delete-event",
					G_CALLBACK(delete_handler),
					inst );

	inst->name_id = g_bus_own_name(	G_BUS_TYPE_SESSION,
					MPRIS_BUS_NAME,
					G_BUS_NAME_OWNER_FLAGS_NONE,
					(GBusAcquiredCallback)
					bus_acquired_handler,
					NULL,
					(GBusNameLostCallback)
					name_lost_handler,
					inst,
					NULL );
}
