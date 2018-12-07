/*
 * Copyright (c) 2015-2017 gnome-mpv
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

#include "gmpv_view.h"
#include "gmpv_mpris.h"
#include "gmpv_mpris_module.h"
#include "gmpv_mpris_base.h"
#include "gmpv_mpris_gdbus.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_CONTROLLER,
	N_PROPERTIES
};


struct _GmpvMprisBase
{
	GmpvMprisModule parent;
	GmpvController *controller;
	guint reg_id;
};

struct _GmpvMprisBaseClass
{
	GmpvMprisModuleClass parent_class;
};

static void register_interface(GmpvMprisModule *module);
static void unregister_interface(GmpvMprisModule *module);

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
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
static void update_fullscreen(GmpvMprisBase *base);
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
	GDBusConnection *conn;

	base = GMPV_MPRIS_BASE(module);
	view = gmpv_controller_get_view(base->controller);

	g_object_get(module, "conn", &conn, "iface", &iface, NULL);

	gmpv_mpris_module_connect_signal
		(	module,
			view,
			"notify::fullscreen",
			G_CALLBACK(fullscreen_handler),
			module );

	gmpv_mpris_module_set_properties
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

	update_fullscreen(GMPV_MPRIS_BASE(module));
}

static void unregister_interface(GmpvMprisModule *module)
{
	GmpvMprisBase *base = GMPV_MPRIS_BASE(module);
	GDBusConnection *conn = NULL;

	g_object_get(module, "conn", &conn, NULL);
	g_dbus_connection_unregister_object(conn, base->reg_id);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMprisBase *self = GMPV_MPRIS_BASE(object);

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

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvMprisBase *self = GMPV_MPRIS_BASE(object);

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
		gmpv_view_present(gmpv_controller_get_view(base->controller));
	}
	else if(g_strcmp0(method_name, "Quit") == 0)
	{
		gmpv_controller_quit(base->controller);
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
	GVariant *value = NULL;

	gmpv_mpris_module_get_properties(data, property_name, &value, NULL);

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
		GmpvView *view = gmpv_controller_get_view(base->controller);

		gmpv_view_set_fullscreen(view, g_variant_get_boolean(value));
	}
	else
	{
		gmpv_mpris_module_set_properties(	data,
							property_name, value,
							NULL );
	}

	return TRUE; /* This function should always succeed */
}

static void fullscreen_handler(	GObject *object,
				GParamSpec *pspec,
				gpointer data )
{
	update_fullscreen(data);
}

static void update_fullscreen(GmpvMprisBase *base)
{
	GmpvMprisModule *module = GMPV_MPRIS_MODULE(base);
	GmpvModel *model = gmpv_controller_get_model(base->controller);
	GVariant *old_value = NULL;
	gboolean fullscreen = FALSE;

	gmpv_mpris_module_get_properties(module, "Fullscreen", &old_value, NULL);
	g_object_get(G_OBJECT(model), "fullscreen", &fullscreen, NULL);

	if(g_variant_get_boolean(old_value) != fullscreen)
	{
		gmpv_mpris_module_set_properties
			(	module,
				"Fullscreen", g_variant_new_boolean(fullscreen),
				NULL );
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
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_pointer
		(	"controller",
			"Controller",
			"The GmpvController to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONTROLLER, pspec);
}

static void gmpv_mpris_base_init(GmpvMprisBase *base)
{
	base->controller = NULL;
	base->reg_id = 0;
}

GmpvMprisModule *gmpv_mpris_base_new(	GmpvController *controller,
					GDBusConnection *conn )
{
	GDBusInterfaceInfo *iface;

	iface = gmpv_mpris_org_mpris_media_player2_interface_info();

	return GMPV_MPRIS_MODULE(g_object_new(	gmpv_mpris_base_get_type(),
						"controller", controller,
						"conn", conn,
						"iface", iface,
						NULL ));
}

