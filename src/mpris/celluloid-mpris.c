/*
 * Copyright (c) 2015-2019 gnome-mpv
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

#include <unistd.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "celluloid-mpris.h"
#include "celluloid-mpris-base.h"
#include "celluloid-mpris-player.h"
#include "celluloid-mpris-track-list.h"
#include "celluloid-def.h"

enum
{
	PROP_0,
	PROP_CONTROLLER,
	N_PROPERTIES
};

struct _CelluloidMpris
{
	GObject parent;
	CelluloidController *controller;
	CelluloidMprisModule *base;
	CelluloidMprisModule *player;
	CelluloidMprisModule *track_list;
	guint name_id;
	GDBusConnection *session_bus_conn;
};

struct _CelluloidMprisClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE(CelluloidMpris, celluloid_mpris, G_TYPE_OBJECT)

static void
constructed(GObject *object);

static void
dispose(GObject *object);

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
name_acquired_handler(	GDBusConnection *connection,
			const gchar *name,
			gpointer data );

static void
name_lost_handler(	GDBusConnection *connection,
			const gchar *name,
			gpointer data );

static void
unregister(CelluloidMpris *mpris);

static void
constructed(GObject *object)
{
	CelluloidMpris *self = CELLULOID_MPRIS(object);
	gchar *address = NULL;
	GDBusConnection *conn =	NULL;

	address =	g_dbus_address_get_for_bus_sync
			(G_BUS_TYPE_SESSION, NULL, NULL);

	if(address)
	{
		const GDBusConnectionFlags flags =
			G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT|
			G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION;

		conn =	g_dbus_connection_new_for_address_sync
			(address, flags, NULL, NULL, NULL);

		g_free(address);
	}

	if(conn)
	{
		CelluloidView *view =	celluloid_controller_get_view
					(self->controller);
		CelluloidMainWindow *window =	celluloid_view_get_main_window
						(view);
		guint window_id =	gtk_application_window_get_id
					(GTK_APPLICATION_WINDOW(window));
		gchar *name =	g_strdup_printf
				(MPRIS_BUS_NAME ".instance-%u", window_id);

		self->name_id =	g_bus_own_name_on_connection
				(	conn,
					name,
					G_BUS_NAME_OWNER_FLAGS_NONE,
					(GBusNameAcquiredCallback)
					name_acquired_handler,
					(GBusNameLostCallback)
					name_lost_handler,
					self,
					NULL );

		g_free(name);
	}
	else
	{
		g_warning(	"Failed to create DBus connection; "
				"MPRIS will be unavailable" );
	}

	G_OBJECT_CLASS(celluloid_mpris_parent_class)->constructed(object);
}

static void
dispose(GObject *object)
{
	CelluloidMpris *self = CELLULOID_MPRIS(object);

	unregister(self);
	g_clear_object(&self->base);
	g_clear_object(&self->player);
	g_clear_object(&self->track_list);
	g_bus_unown_name(self->name_id);

	G_OBJECT_CLASS(celluloid_mpris_parent_class)->dispose(object);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidMpris *self = CELLULOID_MPRIS(object);

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
	CelluloidMpris *self = CELLULOID_MPRIS(object);

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
name_acquired_handler(	GDBusConnection *connection,
			const gchar *name,
			gpointer data )
{
	CelluloidMpris *self = data;

	self->session_bus_conn = connection;
	self->base =		celluloid_mpris_base_new
				(self->controller, connection);
	self->player =		celluloid_mpris_player_new
				(self->controller, connection);
	self->track_list =	celluloid_mpris_track_list_new
				(self->controller, connection);

	celluloid_mpris_module_register(self->base);
	celluloid_mpris_module_register(self->player);
	celluloid_mpris_module_register(self->track_list);
}

static void
name_lost_handler(	GDBusConnection *connection,
			const gchar *name,
			gpointer data )
{
	unregister(data);
}

static void
unregister(CelluloidMpris *mpris)
{
	celluloid_mpris_module_unregister(mpris->base);
	celluloid_mpris_module_unregister(mpris->player);
	celluloid_mpris_module_unregister(mpris->track_list);
}

static void
celluloid_mpris_init(CelluloidMpris *mpris)
{
	mpris->controller = NULL;
	mpris->base = NULL;
	mpris->player = NULL;
	mpris->track_list = NULL;
	mpris->name_id = 0;
	mpris->session_bus_conn = NULL;
}

static void
celluloid_mpris_class_init(CelluloidMprisClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->constructed = constructed;
	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose = dispose;

	pspec = g_param_spec_pointer
		(	"controller",
			"Controller",
			"The CelluloidController to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONTROLLER, pspec);
}

GVariant *
celluloid_mpris_build_g_variant_string_array(const gchar** list)
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

CelluloidMpris *
celluloid_mpris_new(CelluloidController *controller)
{
	return CELLULOID_MPRIS(g_object_new(	celluloid_mpris_get_type(),
						"controller", controller,
						NULL ));
}
