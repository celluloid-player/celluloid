/*
 * Copyright (c) 2015-2021 gnome-mpv
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

#include "celluloid-media-keys.h"
#include "celluloid-def.h"

#include <stdbool.h>

enum
{
	PROP_0,
	PROP_CONTROLLER,
	N_PROPERTIES
};

struct _CelluloidMediaKeys
{
	GObject parent;
	CelluloidController *controller;
	gulong focus_sig_id;
	GDBusProxy *proxy;
	GDBusConnection *session_bus_conn;
};

struct _CelluloidMediaKeysClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE(CelluloidMediaKeys, celluloid_media_keys, G_TYPE_OBJECT)

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
notify_handler(GObject *gobject, GParamSpec *pspec, gpointer data);

static void
g_signal_handler(	GDBusProxy *proxy,
			gchar *sender_name,
			gchar *signal_name,
			GVariant *parameters,
			gpointer data );

static void
proxy_ready_handler(	GObject *source_object,
			GAsyncResult *res,
			gpointer data );

static void
session_ready_handler(	GObject *source_object,
			GAsyncResult *res,
			gpointer data );

static void
constructed(GObject *object)
{
	CelluloidMediaKeys *self = CELLULOID_MEDIA_KEYS(object);
	CelluloidView *view = celluloid_controller_get_view(self->controller);

	self->focus_sig_id =	g_signal_connect
				(	celluloid_view_get_main_window(view),
					"notify::is-active",
					G_CALLBACK(notify_handler),
					self );

	g_bus_get(G_BUS_TYPE_SESSION, NULL, session_ready_handler, self);
}

static void
dispose(GObject *object)
{
	CelluloidMediaKeys *self = CELLULOID_MEDIA_KEYS(object);
	CelluloidView *view = celluloid_controller_get_view(self->controller);
	CelluloidMainWindow *window = celluloid_view_get_main_window(view);

	if(self->focus_sig_id > 0)
	{
		g_signal_handler_disconnect(window, self->focus_sig_id);
		self->focus_sig_id = 0;
	}

	g_clear_object(&self->proxy);
	g_clear_object(&self->session_bus_conn);

	G_OBJECT_CLASS(celluloid_media_keys_parent_class)->dispose(object);
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidMediaKeys *self = CELLULOID_MEDIA_KEYS(object);

	switch(property_id)
	{
		case PROP_CONTROLLER:
		self->controller = g_value_get_pointer(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	G_OBJECT_CLASS(celluloid_media_keys_parent_class)->constructed(object);
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidMediaKeys *self = CELLULOID_MEDIA_KEYS(object);

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
notify_handler(GObject *gobject, GParamSpec *pspec, gpointer data)
{
	CelluloidMediaKeys *self = CELLULOID_MEDIA_KEYS(data);

	if(self->proxy && g_strcmp0(pspec->name, "is-active") == 0)
	{
		gboolean is_active = FALSE;

		g_object_get(gobject, "is-active", &is_active, NULL);

		if(is_active)
		{
			g_dbus_proxy_call(	self->proxy,
						"GrabMediaPlayerKeys",
						g_variant_new("(su)", APP_ID, 0),
						G_DBUS_CALL_FLAGS_NONE,
						-1,
						NULL,
						NULL,
						self );
		}
	}
}

static void
g_signal_handler(	GDBusProxy *proxy,
			gchar *sender_name,
			gchar *signal_name,
			GVariant *parameters,
			gpointer data )
{
	CelluloidMediaKeys *self = data;
	gchar *celluloid_application = NULL;
	gchar *key = NULL;

	if(g_strcmp0(signal_name, "MediaPlayerKeyPressed") == 0)
	{
		g_variant_get
			(parameters, "(&s&s)", &celluloid_application, &key);
	}

	if(g_strcmp0(celluloid_application, APP_ID) == 0)
	{
		const struct
		{
			const char *gsd_key;
			const char *mpv_key;
		}
		dict[] = {	{"Next",	"NEXT"},
				{"Previous",	"PREV"},
				{"Pause",	"PAUSE"},
				{"Stop",	"STOP"},
				{"Play",	"PLAY"},
				{"FastForward","FORWARD"},
				{"Rewind",	"REWIND"},
				{NULL,		NULL} };

		CelluloidModel *model =	celluloid_controller_get_model
					(self->controller);
		bool done = false;

		for(int i = 0; !done && dict[i].gsd_key; i++)
		{
			if(g_strcmp0(dict[i].gsd_key, key) == 0)
			{
				celluloid_model_key_press(model, dict[i].mpv_key);
				done = true;
			}
		}
	}
}

static void
proxy_ready_handler(	GObject *source_object,
			GAsyncResult *res,
			gpointer data )
{
	CelluloidMediaKeys *self = data;
	GError *error = NULL;

	self->proxy = g_dbus_proxy_new_finish(res, &error);

	if(self->proxy)
	{
		g_signal_connect(	self->proxy,
					"g-signal",
					G_CALLBACK(g_signal_handler),
					self );
		g_dbus_proxy_call(	self->proxy,
					"GrabMediaPlayerKeys",
					g_variant_new("(su)", APP_ID, 0),
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL,
					NULL,
					NULL );
	}
	else
	{
		g_error("Failed to create GDBus proxy for media keys");
	}

	if(error)
	{
		g_error("%s", error->message);
		g_error_free(error);
	}
}

static void
session_ready_handler(	GObject *source_object,
			GAsyncResult *res,
			gpointer data )
{
	CelluloidMediaKeys *self = data;

	self->session_bus_conn = g_bus_get_finish(res, NULL);

	g_dbus_proxy_new(	self->session_bus_conn,
				G_DBUS_PROXY_FLAGS_NONE,
				NULL,
				"org.gnome.SettingsDaemon.MediaKeys",
				"/org/gnome/SettingsDaemon/MediaKeys",
				"org.gnome.SettingsDaemon.MediaKeys",
				NULL,
				proxy_ready_handler,
				self );
}

static void
celluloid_media_keys_init(CelluloidMediaKeys *self)
{
	self->controller = NULL;
	self->focus_sig_id = 0;
	self->proxy = NULL;
	self->session_bus_conn = NULL;
}

static void
celluloid_media_keys_class_init(CelluloidMediaKeysClass *klass)
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

CelluloidMediaKeys *
celluloid_media_keys_new(CelluloidController *controller)
{
	GType type = celluloid_media_keys_get_type();

	return CELLULOID_MEDIA_KEYS(g_object_new(	type,
							"controller", controller,
							NULL ));
}
