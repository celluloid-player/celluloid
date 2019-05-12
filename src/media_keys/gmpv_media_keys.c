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

#include "gmpv_media_keys.h"
#include "gmpv_def.h"

#include <stdbool.h>

enum
{
	PROP_0,
	PROP_CONTROLLER,
	N_PROPERTIES
};

struct _GmpvMediaKeys
{
	GObject parent;
	GmpvController *controller;
	gulong focus_sig_id;
	GDBusProxy *proxy;
	GDBusConnection *session_bus_conn;
};

struct _GmpvMediaKeysClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE(GmpvMediaKeys, gmpv_media_keys, G_TYPE_OBJECT)

static void constructed(GObject *object);
static void dispose(GObject *object);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static gboolean window_state_handler( GtkWidget *widget,
				      GdkEventWindowState *event,
				      gpointer data );
static void g_signal_handler(	GDBusProxy *proxy,
				gchar *sender_name,
				gchar *signal_name,
				GVariant *parameters,
				gpointer data );
static void proxy_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data );
static void session_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data );

static void constructed(GObject *object)
{
	GmpvMediaKeys *self = GMPV_MEDIA_KEYS(object);
	GmpvView *view = gmpv_controller_get_view(self->controller);

	self->focus_sig_id =	g_signal_connect
				(	gmpv_view_get_main_window(view),
					"window-state-event",
					G_CALLBACK(window_state_handler),
					self );

	g_bus_get(G_BUS_TYPE_SESSION, NULL, session_ready_handler, self);
}

static void dispose(GObject *object)
{
	GmpvMediaKeys *self = GMPV_MEDIA_KEYS(object);
	GmpvView *view = gmpv_controller_get_view(self->controller);
	GmpvMainWindow *window = gmpv_view_get_main_window(view);

	if(self->focus_sig_id > 0)
	{
		g_signal_handler_disconnect(window, self->focus_sig_id);
		self->focus_sig_id = 0;
	}

	g_clear_object(&self->proxy);
	g_clear_object(&self->session_bus_conn);

	G_OBJECT_CLASS(gmpv_media_keys_parent_class)->dispose(object);
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMediaKeys *self = GMPV_MEDIA_KEYS(object);

	switch(property_id)
	{
		case PROP_CONTROLLER:
		self->controller = g_value_get_pointer(value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	G_OBJECT_CLASS(gmpv_media_keys_parent_class)->constructed(object);
}

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvMediaKeys *self = GMPV_MEDIA_KEYS(object);

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

static gboolean window_state_handler( GtkWidget *widget,
				      GdkEventWindowState *event,
				      gpointer data )
{
	GmpvMediaKeys *self = data;

	if (event->changed_mask & GDK_WINDOW_STATE_FOCUSED
	    && event->new_window_state & GDK_WINDOW_STATE_FOCUSED
	    && self->proxy != NULL)
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

	return FALSE;
}

static void g_signal_handler(	GDBusProxy *proxy,
				gchar *sender_name,
				gchar *signal_name,
				GVariant *parameters,
				gpointer data )
{
	GmpvMediaKeys *self = data;
	gchar *gmpv_application = NULL;
	gchar *key = NULL;

	if(g_strcmp0(signal_name, "MediaPlayerKeyPressed") == 0)
	{
		g_variant_get(parameters, "(&s&s)", &gmpv_application, &key);
	}

	if(g_strcmp0(gmpv_application, APP_ID) == 0)
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

		GmpvModel *model = gmpv_controller_get_model(self->controller);
		bool done = false;

		for(int i = 0; !done && dict[i].gsd_key; i++)
		{
			if(g_strcmp0(dict[i].gsd_key, key) == 0)
			{
				gmpv_model_key_press(model, dict[i].mpv_key);
				done = true;
			}
		}
	}
}

static void proxy_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	GmpvMediaKeys *self = data;
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

static void session_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	GmpvMediaKeys *self = data;

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

static void gmpv_media_keys_init(GmpvMediaKeys *self)
{
	self->controller = NULL;
	self->focus_sig_id = 0;
	self->proxy = NULL;
	self->session_bus_conn = NULL;
}

static void gmpv_media_keys_class_init(GmpvMediaKeysClass *klass)
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
			"The GmpvController to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_CONTROLLER, pspec);
}

GmpvMediaKeys *gmpv_media_keys_new(GmpvController *controller)
{
	return GMPV_MEDIA_KEYS(g_object_new(	gmpv_media_keys_get_type(),
						"controller", controller,
						NULL ));
}
