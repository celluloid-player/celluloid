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

static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data );
static void media_key_press_handler(	GDBusProxy *proxy,
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

static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	gmpv_media_keys *inst = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(inst->gmpv_ctx);

	g_signal_handler_disconnect(wnd, inst->shutdown_sig_id);
	g_signal_handler_disconnect(wnd, inst->focus_sig_id);
	g_signal_handler_disconnect(inst->proxy, inst->g_signal_sig_id);

	g_object_unref(inst->proxy);
	g_object_unref(inst->session_bus_conn);
	g_free(inst);

	return FALSE;
}

static gboolean window_state_handler( GtkWidget *widget,
				      GdkEventWindowState *event,
				      gpointer data )
{
	gmpv_media_keys *inst = data;

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
	gmpv_media_keys *inst = data;
	gchar *gmpv_application = NULL;
	gchar *key = NULL;

	if(g_strcmp0(signal_name, "MediaPlayerKeyPressed") == 0)
	{
		g_variant_get(parameters, "(&s&s)", &gmpv_application, &key);
	}

	if(g_strcmp0(gmpv_application, APP_ID) == 0)
	{
		GmpvModel *model = GMPV_APPLICATION(inst->gmpv_ctx)->model;

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
	gmpv_media_keys *inst = data;

	inst->proxy = g_dbus_proxy_new_finish(res, NULL);

	inst->g_signal_sig_id
		= g_signal_connect(	inst->proxy,
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

static void session_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	gmpv_media_keys *inst = data;

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
				proxy_ready_handler,
				inst );
}

void gmpv_media_keys_init(GmpvApplication *gmpv_ctx)
{
	gmpv_media_keys *inst = g_new0(gmpv_media_keys, 1);
	GmpvMainWindow *wnd = gmpv_application_get_main_window(gmpv_ctx);

	inst->gmpv_ctx = gmpv_ctx;

	inst->shutdown_sig_id =	g_signal_connect
				(	wnd,
					"delete-event",
					G_CALLBACK(delete_handler),
					inst );

	inst->focus_sig_id = g_signal_connect
				(	wnd,
					"window-state-event",
					G_CALLBACK(window_state_handler),
					inst );

	g_bus_get(G_BUS_TYPE_SESSION, NULL, session_ready_handler, inst);
}
