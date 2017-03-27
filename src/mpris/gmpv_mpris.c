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

#include <unistd.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gmpv_mpris.h"
#include "gmpv_mpris_base.h"
#include "gmpv_mpris_player.h"
#include "gmpv_def.h"

static void name_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data );
static void name_lost_handler(	GDBusConnection *connection,
				const gchar *name,
				gpointer data );
static gboolean shutdown_handler(GtkApplication *app, gpointer data);
static void unregister(gmpv_mpris *inst);

static void name_acquired_handler(	GDBusConnection *connection,
					const gchar *name,
					gpointer data )
{
	gmpv_mpris *inst = data;

	inst->session_bus_conn = connection;
	inst->base = GMPV_MPRIS_MODULE(gmpv_mpris_base_new(	inst->gmpv_ctx,
								connection ));
	inst->player = GMPV_MPRIS_MODULE(gmpv_mpris_player_new(	inst->gmpv_ctx,
								connection ));

	gmpv_mpris_module_register_interface(inst->base);
	gmpv_mpris_module_register_interface(inst->player);
}

static void name_lost_handler(	GDBusConnection *connection,
				const gchar *name,
				gpointer data )
{
	unregister(data);
}

static gboolean shutdown_handler(GtkApplication *app, gpointer data)
{
	gmpv_mpris *inst = data;

	g_signal_handler_disconnect(app, inst->shutdown_sig_id);

	unregister(inst);
	g_bus_unown_name(inst->name_id);
	g_free(inst);

	return FALSE;
}

static void unregister(gmpv_mpris *inst)
{
	gmpv_mpris_module_unregister_interface(inst->base);
	gmpv_mpris_module_unregister_interface(inst->player);
}

GVariant *gmpv_mpris_build_g_variant_string_array(const gchar** list)
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

void gmpv_mpris_init(GmpvApplication *gmpv_ctx)
{
	gmpv_mpris *inst;
	gchar *name;

	inst = g_malloc(sizeof(gmpv_mpris));

	/* sizeof(pid_t) can theoretically be larger than sizeof(gint64), but
	 * even then the chance of collision would be minimal.
	 */
	name =	g_strdup_printf
		(	MPRIS_BUS_NAME ".instance%" G_GINT64_FORMAT,
			ABS((gint64)getpid()) );

	inst->gmpv_ctx = gmpv_ctx;
	inst->base = NULL;
	inst->player = NULL;
	inst->name_id = 0;
	inst->shutdown_sig_id = 0;
	inst->pending_seek = -1;
	inst->session_bus_conn = NULL;
	inst->shutdown_sig_id =	g_signal_connect
				(	gmpv_ctx,
					"shutdown",
					G_CALLBACK(shutdown_handler),
					inst );
	inst->name_id = g_bus_own_name(	G_BUS_TYPE_SESSION,
					name,
					G_BUS_NAME_OWNER_FLAGS_NONE,
					NULL,
					(GBusNameAcquiredCallback)
					name_acquired_handler,
					(GBusNameLostCallback)
					name_lost_handler,
					inst,
					NULL );

	g_free(name);
}
