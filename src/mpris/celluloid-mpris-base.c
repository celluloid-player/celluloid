/*
 * Copyright (c) 2015-2019, 2021 gnome-mpv
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
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "celluloid-view.h"
#include "celluloid-mpris.h"
#include "celluloid-mpris-module.h"
#include "celluloid-mpris-base.h"
#include "celluloid-mpris-gdbus.h"
#include "celluloid-def.h"

enum
{
	PROP_0,
	PROP_CONTROLLER,
	N_PROPERTIES
};


struct _CelluloidMprisBase
{
	CelluloidMprisModule parent;
	CelluloidController *controller;
	GHashTable *readonly_table;
	guint reg_id;
};

struct _CelluloidMprisBaseClass
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
fullscreen_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data );

static void
update_fullscreen(CelluloidMprisBase *base);

static GVariant *
get_supported_uri_schemes(void);

static GVariant *
get_supported_mime_types(void);

static void
celluloid_mpris_base_class_init(CelluloidMprisBaseClass *klass);

static void
celluloid_mpris_base_init(CelluloidMprisBase *base);

G_DEFINE_TYPE(CelluloidMprisBase, celluloid_mpris_base, CELLULOID_TYPE_MPRIS_MODULE);

static void
register_interface(CelluloidMprisModule *module)
{
	CelluloidMprisBase *base;
	CelluloidView *view;
	GDBusInterfaceVTable vtable;
	GDBusInterfaceInfo *iface;
	GDBusConnection *conn;

	base = CELLULOID_MPRIS_BASE(module);
	view = celluloid_controller_get_view(base->controller);

	g_object_get(module, "conn", &conn, "iface", &iface, NULL);

	celluloid_mpris_module_connect_signal
		(	module,
			view,
			"notify::fullscreen",
			G_CALLBACK(fullscreen_handler),
			module );

	celluloid_mpris_module_set_properties
		(	module,
			"CanQuit", g_variant_new_boolean(TRUE),
			"CanSetFullscreen", g_variant_new_boolean(TRUE),
			"CanRaise", g_variant_new_boolean(TRUE),
			"Fullscreen", g_variant_new_boolean(FALSE),
			"HasTrackList", g_variant_new_boolean(TRUE),
			"Identity", g_variant_new_string(g_get_application_name()),
			"DesktopEntry", g_variant_new_string(APP_ID),
			"SupportedUriSchemes", get_supported_uri_schemes(),
			"SupportedMimeTypes", get_supported_mime_types(),
			NULL );

	vtable.method_call = (GDBusInterfaceMethodCallFunc)method_handler;
	vtable.get_property = (GDBusInterfaceGetPropertyFunc)get_prop_handler;
	vtable.set_property = (GDBusInterfaceSetPropertyFunc)set_prop_handler;

	base->reg_id =	g_dbus_connection_register_object
			(	conn,
				MPRIS_OBJ_ROOT_PATH,
				iface,
				&vtable,
				module,
				NULL,
				NULL );

	update_fullscreen(CELLULOID_MPRIS_BASE(module));
}

static void
unregister_interface(CelluloidMprisModule *module)
{
	CelluloidMprisBase *base = CELLULOID_MPRIS_BASE(module);
	GDBusConnection *conn = NULL;

	g_object_get(module, "conn", &conn, NULL);
	g_dbus_connection_unregister_object(conn, base->reg_id);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidMprisBase *self = CELLULOID_MPRIS_BASE(object);

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
	CelluloidMprisBase *self = CELLULOID_MPRIS_BASE(object);

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
method_handler(	GDBusConnection *connection,
		const gchar *sender,
		const gchar *object_path,
		const gchar *interface_name,
		const gchar *method_name,
		GVariant *parameters,
		GDBusMethodInvocation *invocation,
		gpointer data )
{
	CelluloidMprisBase *base = data;
	gboolean unknown_method = FALSE;

	if(g_strcmp0(method_name, "Raise") == 0)
	{
		celluloid_view_present
			(celluloid_controller_get_view(base->controller));
	}
	else if(g_strcmp0(method_name, "Quit") == 0)
	{
		celluloid_controller_quit(base->controller);
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
	CelluloidMprisBase *base = CELLULOID_MPRIS_BASE(data);
	CelluloidMprisModule *module = CELLULOID_MPRIS_MODULE(data);
	GVariant *value = NULL;

	if(!g_hash_table_contains(base->readonly_table, property_name))
	{
		g_set_error
			(	error,
				CELLULOID_MPRIS_ERROR,
				CELLULOID_MPRIS_ERROR_UNKNOWN_PROPERTY,
				"Failed to get value of unknown property \"%s\"",
				property_name );
	}
	else
	{
		celluloid_mpris_module_get_properties
			(module, property_name, &value, NULL);
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
	CelluloidMprisBase *base = CELLULOID_MPRIS_BASE(data);
	gboolean result = TRUE;

	if(!g_hash_table_contains(base->readonly_table, property_name))
	{
		result = FALSE;

		g_set_error
			(	error,
				CELLULOID_MPRIS_ERROR,
				CELLULOID_MPRIS_ERROR_UNKNOWN_PROPERTY,
				"Failed to set value of unknown property \"%s\"",
				property_name );
	}
	else if(GPOINTER_TO_INT(g_hash_table_lookup(base->readonly_table, property_name)))
	{
		result = FALSE;

		g_set_error
			(	error,
				CELLULOID_MPRIS_ERROR,
				CELLULOID_MPRIS_ERROR_SET_READONLY,
				"Attempted to set value of readonly property \"%s\"",
				property_name );
	}
	else if(g_strcmp0(property_name, "Fullscreen") == 0)
	{
		CelluloidView *view = celluloid_controller_get_view(base->controller);

		celluloid_view_set_fullscreen(view, g_variant_get_boolean(value));
	}

	return result;
}

static void
fullscreen_handler(	GObject *object,
			GParamSpec *pspec,
			gpointer data )
{
	update_fullscreen(data);
}

static void
update_fullscreen(CelluloidMprisBase *base)
{
	CelluloidMprisModule *module = CELLULOID_MPRIS_MODULE(base);
	CelluloidModel *model = celluloid_controller_get_model(base->controller);
	GVariant *old_value = NULL;
	gboolean fullscreen = FALSE;

	celluloid_mpris_module_get_properties
		(module, "Fullscreen", &old_value, NULL);

	g_object_get(G_OBJECT(model), "fullscreen", &fullscreen, NULL);

	if(g_variant_get_boolean(old_value) != fullscreen)
	{
		celluloid_mpris_module_set_properties
			(	module,
				"Fullscreen", g_variant_new_boolean(fullscreen),
				NULL );
	}
}

static GVariant *
get_supported_uri_schemes(void)
{
	const gchar *protocols[] = SUPPORTED_PROTOCOLS;

	return celluloid_mpris_build_g_variant_string_array(protocols);
}

static GVariant *
get_supported_mime_types(void)
{
	const gchar *mime_types[] = SUPPORTED_MIME_TYPES;

	return celluloid_mpris_build_g_variant_string_array(mime_types);
}

static void
celluloid_mpris_base_class_init(CelluloidMprisBaseClass *klass)
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
			"The CelluloidController to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONTROLLER, pspec);
}

static void
celluloid_mpris_base_init(CelluloidMprisBase *base)
{
	const struct
	{
		const gchar *name;
		gboolean readonly;
	}
	properties[] =
	{
		{"CanQuit", TRUE},
		{"Fullscreen", FALSE},
		{"CanSetFullscreen", TRUE},
		{"CanRaise", TRUE},
		{"HasTrackList", TRUE},
		{"Identity", TRUE},
		{"DesktopEntry", TRUE},
		{"SupportedUriSchemes", TRUE},
		{"SupportedMimeTypes", TRUE},
		{NULL, FALSE}
	};

	base->controller =
		NULL;
	base->readonly_table =
		g_hash_table_new_full(g_str_hash, g_int_equal, g_free, NULL);
	base->reg_id =
		0;

	for(gint i = 0; properties[i].name; i++)
	{
		g_hash_table_replace
			(	base->readonly_table,
				g_strdup(properties[i].name),
				GINT_TO_POINTER(properties[i].readonly) );
	}
}

CelluloidMprisModule *
celluloid_mpris_base_new(CelluloidController *controller, GDBusConnection *conn)
{
	GType type;
	GDBusInterfaceInfo *iface;

	type = celluloid_mpris_base_get_type();
	iface = celluloid_mpris_org_mpris_media_player2_interface_info();

	return CELLULOID_MPRIS_MODULE(g_object_new(	type,
							"controller", controller,
							"conn", conn,
							"iface", iface,
							NULL ));
}

