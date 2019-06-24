/*
 * Copyright (c) 2017-2019 gnome-mpv
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

#include "celluloid-mpris-module.h"
#include "celluloid-application.h"
#include "celluloid-def.h"

typedef struct _CelluloidSignalHandlerInfo CelluloidSignalHandlerInfo;
typedef struct _CelluloidMprisModulePrivate CelluloidMprisModulePrivate;

enum
{
	PROP_0,
	PROP_CONN,
	PROP_IFACE,
	N_PROPERTIES
};

struct _CelluloidSignalHandlerInfo
{
	gpointer instance;
	gulong id;
};

struct _CelluloidMprisModulePrivate
{
	GDBusConnection *conn;
	GDBusInterfaceInfo *iface;
	GSList *signal_ids;
	GHashTable *prop_table;
};

G_DEFINE_TYPE_WITH_PRIVATE(CelluloidMprisModule, celluloid_mpris_module, G_TYPE_OBJECT)

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
dispose(GObject *object);

static void
finalize(GObject *object);

static void
disconnect_signal(CelluloidSignalHandlerInfo *info, gpointer data);

static void
celluloid_mpris_module_class_init(CelluloidMprisModuleClass *klass);

static void
celluloid_mpris_module_init(CelluloidMprisModule *module);

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidMprisModulePrivate *priv;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	object,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );

	switch(property_id)
	{
		case PROP_CONN:
		priv->conn = g_value_get_pointer(value);
		break;

		case PROP_IFACE:
		priv->iface = g_value_get_pointer(value);
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
	CelluloidMprisModulePrivate *priv;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	object,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );

	switch(property_id)
	{
		case PROP_CONN:
		g_value_set_pointer(value, priv->conn);
		break;

		case PROP_IFACE:
		g_value_set_pointer(value, priv->iface);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
dispose(GObject *object)
{
	CelluloidMprisModulePrivate *priv;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	object,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );

	g_hash_table_unref(priv->prop_table);

	G_OBJECT_CLASS(celluloid_mpris_module_parent_class)->dispose(object);
}

static void
finalize(GObject *object)
{
	CelluloidMprisModulePrivate *priv;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	object,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );

	g_slist_foreach(priv->signal_ids, (GFunc)disconnect_signal, NULL);
	g_slist_free_full(priv->signal_ids, g_free);

	G_OBJECT_CLASS(celluloid_mpris_module_parent_class)->finalize(object);
}

static void
disconnect_signal(CelluloidSignalHandlerInfo *info, gpointer data)
{
	g_signal_handler_disconnect(info->instance, info->id);
}

static void
celluloid_mpris_module_class_init(CelluloidMprisModuleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	object_class->set_property = set_property;
	object_class->get_property = get_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;

	pspec = g_param_spec_pointer
		(	"conn",
			"Connection",
			"Connection to the session bus",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONN, pspec);

	pspec = g_param_spec_pointer
		(	"iface",
			"Interface",
			"The GDBusInterfaceInfo of the interface",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_IFACE, pspec);
}

static void
celluloid_mpris_module_init(CelluloidMprisModule *module)
{
	CelluloidMprisModulePrivate *priv;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	module,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );

	priv->conn = NULL;
	priv->iface = NULL;
	priv->signal_ids = NULL;
	priv->prop_table =	g_hash_table_new_full
				(	g_str_hash,
					g_str_equal,
					g_free,
					(GDestroyNotify)
					g_variant_unref );
}

void
celluloid_mpris_module_connect_signal(	CelluloidMprisModule *module,
					gpointer instance,
					const gchar *detailed_signal,
					GCallback handler,
					gpointer data )
{
	CelluloidMprisModulePrivate *priv;
	CelluloidSignalHandlerInfo *info;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	module,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );
	info = g_malloc(sizeof(CelluloidSignalHandlerInfo));

	info->instance = instance;
	info->id = g_signal_connect(instance, detailed_signal, handler, data);

	priv->signal_ids = g_slist_prepend(priv->signal_ids, info);
}

void
celluloid_mpris_module_get_properties(CelluloidMprisModule *module, ...)
{
	CelluloidMprisModulePrivate *priv;
	va_list arg;
	gchar *name;
	GVariant **value_ptr;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	module,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );

	va_start(arg, module);

	for(	name = va_arg(arg, gchar *),
		value_ptr = va_arg(arg, GVariant **);
		name && value_ptr;
		name = va_arg(arg, gchar *),
		value_ptr = va_arg(arg, GVariant **) )
	{
		*value_ptr = g_hash_table_lookup(priv->prop_table, name);
	}
}

void
celluloid_mpris_module_set_properties_full(	CelluloidMprisModule *module,
						gboolean send_new_value,
						... )
{
	CelluloidMprisModulePrivate *priv;
	GVariantBuilder builder;
	va_list arg;
	gchar *name;
	GVariant *value;
	const gchar *builder_type_string;
	const gchar *elem_type_string;
	GVariant *sig_args;

	priv =	G_TYPE_INSTANCE_GET_PRIVATE(	module,
						CELLULOID_TYPE_MPRIS_MODULE,
						CelluloidMprisModulePrivate );
	builder_type_string = send_new_value?"a{sv}":"as";
	elem_type_string = builder_type_string+1;

	g_debug("Preparing property change event");
	g_variant_builder_init(&builder, G_VARIANT_TYPE(builder_type_string));

	va_start(arg, send_new_value);

	for(	name = va_arg(arg, gchar *),
		value = va_arg(arg, GVariant *);
		name && value;
		name = va_arg(arg, gchar *),
		value = va_arg(arg, GVariant *) )
	{
		g_hash_table_replace(	priv->prop_table,
					g_strdup(name),
					g_variant_ref_sink(value) );

		g_debug("Adding property \"%s\"", name);
		g_variant_builder_add(&builder, elem_type_string, name, value);
	}

	sig_args = g_variant_new(	"(sa{sv}as)",
					priv->iface->name,
					send_new_value?&builder:NULL,
					send_new_value?NULL:&builder );

	g_debug(	"Emitting property change event on interface %s",
			priv->iface->name );
	g_dbus_connection_emit_signal
		(	priv->conn,
			NULL,
			MPRIS_OBJ_ROOT_PATH,
			"org.freedesktop.DBus.Properties",
			"PropertiesChanged",
			sig_args,
			NULL );
}

void
celluloid_mpris_module_register(CelluloidMprisModule *module)
{
	if(module)
	{
		CelluloidMprisModuleClass *klass;

		g_return_if_fail(CELLULOID_IS_MPRIS_MODULE(module));
		klass = CELLULOID_MPRIS_MODULE_GET_CLASS(module);

		g_return_if_fail(klass->register_interface);
		klass->register_interface(module);
	}
}

void
celluloid_mpris_module_unregister(CelluloidMprisModule *module)
{
	if(module)
	{
		CelluloidMprisModuleClass *klass;

		g_return_if_fail(CELLULOID_IS_MPRIS_MODULE(module));
		klass = CELLULOID_MPRIS_MODULE_GET_CLASS(module);

		g_return_if_fail(klass->unregister_interface);
		klass->unregister_interface(module);
	}
}
