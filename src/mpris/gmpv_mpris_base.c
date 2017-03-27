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
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gmpv_application_private.h"
#include "gmpv_view.h"
#include "gmpv_mpris.h"
#include "gmpv_mpris_module.h"
#include "gmpv_mpris_base.h"
#include "gmpv_mpris_gdbus.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_APP,
	PROP_CONN,
	N_PROPERTIES
};


struct _GmpvMprisBase
{
	GmpvMprisModule parent;
	GmpvApplication *app;
	GDBusConnection *conn;
	guint reg_id;
	GHashTable *prop_table;
	gulong fullscreen_signal_id;
};

struct _GmpvMprisBaseClass
{
	GmpvMprisModuleClass parent_class;
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
static void fullscreen_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data );
static GVariant *get_supported_uri_schemes(void);
static GVariant *get_supported_mime_types(void);
static void gmpv_mpris_base_class_init(GmpvMprisBaseClass *klass);
static void gmpv_mpris_base_init(GmpvMprisBase *base);

G_DEFINE_TYPE(GmpvMprisBase, gmpv_mpris_base, GMPV_TYPE_MPRIS_MODULE);

static void register_interface(GmpvMprisModule *module)
{
	GmpvMprisBase *base;
	GmpvView *view;
	GDBusInterfaceVTable vtable;
	GDBusInterfaceInfo *iface;

	base = GMPV_MPRIS_BASE(module);
	view = base->app->view;
	iface = gmpv_mpris_org_mpris_media_player2_interface_info();

	base->prop_table =	g_hash_table_new_full
				(	g_str_hash,
					g_str_equal,
					NULL,
					(GDestroyNotify)g_variant_unref );

	base->fullscreen_signal_id =	g_signal_connect
					(	view,
						"notify::fullscreen",
						G_CALLBACK(fullscreen_handler),
						module );

	prop_table_init(base->prop_table);

	vtable.method_call = (GDBusInterfaceMethodCallFunc)method_handler;
	vtable.get_property = (GDBusInterfaceGetPropertyFunc)get_prop_handler;
	vtable.set_property = (GDBusInterfaceSetPropertyFunc)set_prop_handler;

	base->reg_id =	g_dbus_connection_register_object
			(	base->conn,
				MPRIS_OBJ_ROOT_PATH,
				iface,
				&vtable,
				module,
				NULL,
				NULL );
}

static void unregister_interface(GmpvMprisModule *module)
{
	GmpvMprisBase *base = GMPV_MPRIS_BASE(module);

	g_signal_handler_disconnect(base->app->view, base->fullscreen_signal_id);
	g_dbus_connection_unregister_object(base->conn, base->reg_id);
	g_hash_table_remove_all(base->prop_table);
	g_hash_table_unref(base->prop_table);
}

static void constructed(GObject *object)
{
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMprisBase *self = GMPV_MPRIS_BASE(object);

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
	GmpvMprisBase *self = GMPV_MPRIS_BASE(object);

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
	const gpointer default_values[]
		= {	"CanQuit", g_variant_new_boolean(TRUE),
			"CanSetFullscreen", g_variant_new_boolean(TRUE),
			"CanRaise", g_variant_new_boolean(TRUE),
			"Fullscreen", g_variant_new_boolean(FALSE),
			"HasTrackList", g_variant_new_boolean(FALSE),
			"Identity", g_variant_new_string(g_get_application_name()),
			"DesktopEntry", g_variant_new_string(APP_ID),
			"SupportedUriSchemes", get_supported_uri_schemes(),
			"SupportedMimeTypes", get_supported_mime_types(),
			NULL };

	for(gint i = 0; default_values[i]; i += 2)
	{
		g_hash_table_insert(	table,
					default_values[i],
					default_values[i+1] );
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
	GmpvMprisBase *base = data;

	if(g_strcmp0(method_name, "Raise") == 0)
	{
		gmpv_view_present(base->app->view);
	}
	else if(g_strcmp0(method_name, "Quit") == 0)
	{
		gmpv_application_quit(base->app);
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
	GmpvMprisBase *base = data;
	GVariant *value;

	value = g_hash_table_lookup(base->prop_table, property_name);

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
	GmpvMprisBase *base = data;

	if(g_strcmp0(property_name, "Fullscreen") == 0)
	{
		gmpv_view_set_fullscreen
			(base->app->view, g_variant_get_boolean(value));
	}
	else
	{
		g_hash_table_replace(	GMPV_MPRIS_BASE(data)->prop_table,
					g_strdup(property_name),
					g_variant_ref(value) );
	}

	return TRUE; /* This function should always succeed */
}

static void fullscreen_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	GmpvMprisBase *base = data;
	GVariant *old_value = NULL;
	gboolean fullscreen = FALSE;

	old_value = g_hash_table_lookup(	base->prop_table,
						g_strdup("Fullscreen") );

	g_object_get(object, "fullscreen", &fullscreen, NULL);

	if(g_variant_get_boolean(old_value) != fullscreen)
	{
		GDBusInterfaceInfo *iface;
		GVariant *value;
		GmpvMprisProperty *prop_list;

		iface = gmpv_mpris_org_mpris_media_player2_interface_info();
		value =	g_variant_new_boolean(fullscreen);

		g_hash_table_replace(	base->prop_table,
					g_strdup("Fullscreen"),
					g_variant_ref(value) );

		prop_list =	(GmpvMprisProperty[])
				{	{"Fullscreen", value},
					{NULL, NULL} };

		gmpv_mpris_module_emit_properties_changed(	base->conn,
								iface->name,
								prop_list );
	}

}

static GVariant *get_supported_uri_schemes(void)
{
	const gchar *protocols[] = SUPPORTED_PROTOCOLS;

	return gmpv_mpris_build_g_variant_string_array(protocols);
}

static GVariant *get_supported_mime_types(void)
{
	const gchar *mime_types[] = SUPPORTED_MIME_TYPES;

	return gmpv_mpris_build_g_variant_string_array(mime_types);
}

static void gmpv_mpris_base_class_init(GmpvMprisBaseClass *klass)
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

static void gmpv_mpris_base_init(GmpvMprisBase *base)
{
	base->app = NULL;
	base->conn = NULL;
	base->reg_id = 0;
	base->prop_table = g_hash_table_new(g_str_hash, g_str_equal);
	base->fullscreen_signal_id = 0;
}

GmpvMprisBase *gmpv_mpris_base_new(GmpvApplication *app, GDBusConnection *conn)
{
	return GMPV_MPRIS_BASE(g_object_new(	gmpv_mpris_base_get_type(),
						"app", app,
						"conn", conn,
						NULL ));
}

