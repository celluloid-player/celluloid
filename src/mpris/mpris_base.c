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

#include "config.h"

#include "mpris_base.h"
#include "mpris_gdbus.h"
#include "def.h"

static void prop_table_init(mpris *inst);
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
static gboolean window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );
static GVariant *get_supported_uri_schemes(void);
static GVariant *get_supported_mime_types(void);

static void prop_table_init(mpris *inst)
{
	const gpointer default_values[]
		= {	"CanQuit", g_variant_new_boolean(TRUE),
			"CanSetFullscreen", g_variant_new_boolean(TRUE),
			"CanRaise", g_variant_new_boolean(TRUE),
			"Fullscreen", g_variant_new_boolean(FALSE),
			"HasTrackList", g_variant_new_boolean(FALSE),
			"Identity", g_variant_new_string(g_get_application_name()),
			"DesktopEntry", g_variant_new_string(ICON_NAME),
			"SupportedUriSchemes", get_supported_uri_schemes(),
			"SupportedMimeTypes", get_supported_mime_types(),
			NULL };

	gint i;

	for(i = 0; default_values[i]; i += 2)
	{
		g_hash_table_insert(	inst->base_prop_table,
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
	mpris *inst = data;

	if(g_strcmp0(method_name, "Raise") == 0)
	{
		gtk_window_present(GTK_WINDOW(inst->gmpv_ctx->gui));
	}
	else if(g_strcmp0(method_name, "Quit") == 0)
	{
		quit(inst->gmpv_ctx);
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

	value = g_hash_table_lookup(	inst->base_prop_table,
					property_name );

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

	if(g_strcmp0(property_name, "Fullscreen") == 0
	&& g_variant_get_boolean(value) != inst->gmpv_ctx->gui->fullscreen)
	{
		toggle_fullscreen(inst->gmpv_ctx);
	}
	else
	{
		g_hash_table_replace(	((mpris *) data)->base_prop_table,
					g_strdup(property_name),
					g_variant_ref(value) );
	}

	return TRUE; /* This function should always succeed */
}

static gboolean window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	mpris *inst = data;
	GdkEventWindowState *window_state_event = (GdkEventWindowState *)event;

	if(window_state_event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
	{
		GDBusInterfaceInfo *iface;
		GVariantBuilder builder;
		GVariant *value;
		GVariant *sig_args;

		g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

		iface = mpris_org_mpris_media_player2_interface_info();
		value = g_variant_new_boolean(inst->gmpv_ctx->gui->fullscreen);

		g_variant_builder_add(&builder, "{sv}", "Fullscreen", value);

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

		g_hash_table_replace(	((mpris *) data)->base_prop_table,
					g_strdup("Fullscreen"),
					g_variant_ref(value) );
	}

	return FALSE;
}

static GVariant *get_supported_uri_schemes(void)
{
	const gchar *protocols[] = SUPPORTED_PROTOCOLS;

	return mpris_build_g_variant_string_array(protocols);
}

static GVariant *get_supported_mime_types(void)
{
	const gchar *mime_types[] = SUPPORTED_MIME_TYPES;

	return mpris_build_g_variant_string_array(mime_types);
}

void mpris_base_register(mpris *inst)
{
	GDBusInterfaceVTable vtable;
	GDBusInterfaceInfo *iface;

	iface = mpris_org_mpris_media_player2_interface_info();

	inst->base_prop_table = g_hash_table_new_full
					(	g_str_hash,
						g_str_equal,
						NULL,
						(GDestroyNotify)
						g_variant_unref );

	inst->base_sig_id_list = g_malloc(2*sizeof(guint));

	inst->base_sig_id_list[0]
		= g_signal_connect(	inst->gmpv_ctx->gui,
					"window-state-event",
					G_CALLBACK(window_state_handler),
					inst );

	inst->base_sig_id_list[1] = 0;

	prop_table_init(inst);

	vtable.method_call = (GDBusInterfaceMethodCallFunc)method_handler;
	vtable.get_property = (GDBusInterfaceGetPropertyFunc)get_prop_handler;
	vtable.set_property = (GDBusInterfaceSetPropertyFunc)set_prop_handler;

	inst->base_reg_id
		= g_dbus_connection_register_object
			(	inst->session_bus_conn,
				MPRIS_OBJ_ROOT_PATH,
				iface,
				&vtable,
				inst,
				NULL,
				NULL );
}

void mpris_base_unregister(mpris *inst)
{
	guint *current_sig_id = inst->base_sig_id_list;

	while(current_sig_id && *current_sig_id > 0)
	{
		g_signal_handler_disconnect(	inst->gmpv_ctx->gui,
						*current_sig_id );

		current_sig_id++;
	}

	g_dbus_connection_unregister_object(	inst->session_bus_conn,
						inst->base_reg_id );

	g_hash_table_remove_all(inst->base_prop_table);
	g_hash_table_unref(inst->base_prop_table);
	g_free(inst->base_sig_id_list);
}
