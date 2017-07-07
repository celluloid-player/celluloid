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

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gmpv_application_private.h"
#include "gmpv_main_window.h"
#include "gmpv_media_keys.h"
#include "gmpv_model.h"
#include "gmpv_def.h"

enum
{
	PROP_0,
	PROP_APP,
	N_PROPERTIES
};

struct _GmpvMediaKeys
{
	GObject parent;
	GmpvApplication *app;
	gulong focus_sig_id;
	GDBusProxy *proxy;
	GDBusProxy *compat_proxy;
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
static void media_key_press_handler(	GDBusProxy *proxy,
					gchar *sender_name,
					gchar *signal_name,
					GVariant *parameters,
					gpointer data );
static void proxy_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data );
static void compat_proxy_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data );
static void session_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data );

static void constructed(GObject *object)
{
	GmpvMediaKeys *self = GMPV_MEDIA_KEYS(object);

	self->focus_sig_id =	g_signal_connect
				(	self->app->gui,
					"window-state-event",
					G_CALLBACK(window_state_handler),
					self );

	g_bus_get(G_BUS_TYPE_SESSION, NULL, session_ready_handler, self);
}

static void dispose(GObject *object)
{
	GmpvMediaKeys *self = GMPV_MEDIA_KEYS(object);

	if(self->focus_sig_id > 0)
	{
		g_signal_handler_disconnect(self->app->gui, self->focus_sig_id);
		self->focus_sig_id = 0;
	}

	g_clear_object(&self->proxy);
	g_clear_object(&self->compat_proxy);
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
		case PROP_APP:
		self->app = g_value_get_pointer(value);
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
		case PROP_APP:
		g_value_set_pointer(value, self->app);
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
	GmpvMediaKeys *inst = data;

	if (event->changed_mask & GDK_WINDOW_STATE_FOCUSED
	    && event->new_window_state & GDK_WINDOW_STATE_FOCUSED
	    && inst->proxy != NULL)
	{
		g_dbus_proxy_call(	inst->proxy,
					"GrabMediaPlayerKeys",
					g_variant_new("(su)", APP_ID, 0),
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL,
					NULL,
					inst );
	}

	return FALSE;
}

static void media_key_press_handler(	GDBusProxy *proxy,
					gchar *sender_name,
					gchar *signal_name,
					GVariant *parameters,
					gpointer data )
{
	GmpvMediaKeys *inst = data;
	gchar *gmpv_application = NULL;
	gchar *key = NULL;

	if(g_strcmp0(signal_name, "MediaPlayerKeyPressed") == 0)
	{
		g_variant_get(parameters, "(&s&s)", &gmpv_application, &key);
	}

	if(g_strcmp0(gmpv_application, APP_ID) == 0)
	{
		GmpvModel *model = inst->app->model;

		if(g_strcmp0(key, "Next") == 0)
		{
			gmpv_model_next_playlist_entry(model);
		}
		else if(g_strcmp0(key, "Previous") == 0)
		{
			gmpv_model_previous_playlist_entry(model);
		}
		else if(g_strcmp0(key, "Pause") == 0)
		{
			gmpv_model_pause(model);
		}
		else if(g_strcmp0(key, "Stop") == 0)
		{
			gmpv_model_stop(model);
		}
		else if(g_strcmp0(key, "Play") == 0)
		{
			gboolean pause = FALSE;

			g_object_get(model, "pause", &pause, NULL);
			(pause?gmpv_model_play:gmpv_model_pause)(model);
		}
		else if(g_strcmp0(key, "FastForward") == 0)
		{
			gmpv_model_forward(model);
		}
		else if(g_strcmp0(key, "Rewind") == 0)
		{
			gmpv_model_rewind(model);
		}
	}
}

static void proxy_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	GmpvMediaKeys *inst = data;

	inst->proxy = g_dbus_proxy_new_finish(res, NULL);

	g_signal_connect(	inst->proxy,
				"g-signal",
				G_CALLBACK(media_key_press_handler),
				inst );
	g_dbus_proxy_call(	inst->proxy,
				"GrabMediaPlayerKeys",
				g_variant_new("(su)", APP_ID, 0),
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				NULL,
				NULL );
}

static void compat_proxy_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	GmpvMediaKeys *inst = data;

	inst->compat_proxy = g_dbus_proxy_new_finish(res, NULL);

	g_signal_connect(	inst->proxy,
				"g-signal",
				G_CALLBACK(media_key_press_handler),
				inst );
	g_dbus_proxy_call(	inst->compat_proxy,
				"GrabMediaPlayerKeys",
				g_variant_new("(su)", APP_ID, 0),
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				NULL,
				NULL );
}

static void session_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	GmpvMediaKeys *inst = data;

	inst->session_bus_conn = g_bus_get_finish(res, NULL);

	/* The MediaKeys plugin for gnome-settings-daemon <= 3.24.1 used the
	 * bus name org.gnome.SettingsDaemon despite the documentation stating
	 * that org.gnome.SettingsDaemon.MediaKeys should be used.
	 * gnome-settings-daemon > 3.24.1 changed the bus name to match the
	 * documentation. To remain compatible with older versions, create
	 * proxies for both names.
	 */
	g_dbus_proxy_new(	inst->session_bus_conn,
				G_DBUS_PROXY_FLAGS_NONE,
				NULL,
				"org.gnome.SettingsDaemon.MediaKeys",
				"/org/gnome/SettingsDaemon/MediaKeys",
				"org.gnome.SettingsDaemon.MediaKeys",
				NULL,
				proxy_ready_handler,
				inst );
	g_dbus_proxy_new(	inst->session_bus_conn,
				G_DBUS_PROXY_FLAGS_NONE,
				NULL,
				"org.gnome.SettingsDaemon",
				"/org/gnome/SettingsDaemon/MediaKeys",
				"org.gnome.SettingsDaemon.MediaKeys",
				NULL,
				compat_proxy_ready_handler,
				inst );
}

static void gmpv_media_keys_init(GmpvMediaKeys *media_keys)
{
	media_keys->app = NULL;
	media_keys->focus_sig_id = 0;
	media_keys->proxy = NULL;
	media_keys->compat_proxy = NULL;
	media_keys->session_bus_conn = NULL;
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
		(	"app",
			"App",
			"The GmpvApplication to use",
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(object_class, PROP_APP, pspec);
}

GmpvMediaKeys *gmpv_media_keys_new(GmpvApplication *app)
{
	return GMPV_MEDIA_KEYS(g_object_new(	gmpv_media_keys_get_type(),
						"app", app,
						NULL ));
}
