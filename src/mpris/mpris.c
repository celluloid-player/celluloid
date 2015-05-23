/*
 * Copyright (c) 2015 gnome-mpv
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
#include "mpris_gdbus.h" /* auto-generated */
#include "def.h"

static void bus_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data );
static void shutdown_handler(GtkApplication *application, gpointer data);

static void bus_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data )
{
	mpris *inst = data;

	inst->session_bus_conn = connection;
	inst->base_reg_id = mpris_base_register(inst, connection);
}

static void shutdown_handler(GtkApplication *application, gpointer data)
{
	g_bus_unown_name(((mpris *) data)->name_id);
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

void mpris_init(gmpv_handle *gmpv_ctx)
{
	mpris *inst = g_malloc(sizeof(mpris));

	inst->gmpv_ctx = gmpv_ctx;
	inst->base_prop_table = g_hash_table_new(g_str_hash, g_str_equal);

	mpris_base_prop_table_init(inst);

	g_signal_connect(	inst->gmpv_ctx->app,
				"shutdown",
				G_CALLBACK(shutdown_handler),
				inst );

	inst->name_id = g_bus_own_name(	G_BUS_TYPE_SESSION,
					MPRIS_BUS_NAME,
					G_BUS_NAME_OWNER_FLAGS_NONE,
					(GBusAcquiredCallback)
					bus_acquired_handler,
					NULL,
					NULL,
					inst,
					NULL );
}
