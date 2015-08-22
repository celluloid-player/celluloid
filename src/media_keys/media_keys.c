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

#include "media_keys.h"
#include "def.h"

static void set_paused(gmpv_handle *ctx, gboolean paused);
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

static void set_paused(gmpv_handle *ctx, gboolean paused)
{
	ctx->paused = paused;

	mpv_set_property(	ctx->mpv_ctx,
				"pause",
				MPV_FORMAT_FLAG,
				&ctx->paused );
}

static gboolean delete_handler(	GtkWidget *widget,
				GdkEvent *event,
				gpointer data )
{
	media_keys *inst = data;

	g_signal_handler_disconnect(	inst->gmpv_ctx->gui,
					inst->shutdown_sig_id );

	g_signal_handler_disconnect(	inst->proxy,
					inst->g_signal_sig_id );

	g_object_unref(inst->proxy);
	g_object_unref(inst->session_bus_conn);
	g_free(inst);

	return FALSE;
}

static void media_key_press_handler(	GDBusProxy *proxy,
					gchar *sender_name,
					gchar *signal_name,
					GVariant *parameters,
					gpointer data )
{
	media_keys *inst = data;
	gchar *application = NULL;
	gchar *key = NULL;

	if(g_strcmp0(signal_name, "MediaPlayerKeyPressed") == 0)
	{
		g_variant_get(parameters, "(&s&s)", &application, &key);
	}

	if(g_strcmp0(application, APP_ID) == 0)
	{
		if(g_strcmp0(key, "Next") == 0)
		{
			const gchar *cmd[] = {"playlist_next", "weak", NULL};

			mpv_command(inst->gmpv_ctx->mpv_ctx, cmd);
		}
		else if(g_strcmp0(key, "Previous") == 0)
		{
			const gchar *cmd[] = {"playlist_prev", "weak", NULL};

			mpv_command(inst->gmpv_ctx->mpv_ctx, cmd);
		}
		else if(g_strcmp0(key, "Pause") == 0)
		{
			set_paused(inst->gmpv_ctx, TRUE);
		}
		else if(g_strcmp0(key, "Stop") == 0)
		{
			const gchar *cmd[] = {"stop", NULL};

			mpv_command(inst->gmpv_ctx->mpv_ctx, cmd);
		}
		else if(g_strcmp0(key, "Play") == 0)
		{
			set_paused(inst->gmpv_ctx, !inst->gmpv_ctx->paused);
		}
		else if(g_strcmp0(key, "FastForward") == 0)
		{
			const gchar *cmd[] = {"seek", "10", NULL};

			mpv_command(inst->gmpv_ctx->mpv_ctx, cmd);
		}
		else if(g_strcmp0(key, "Rewind") == 0)
		{
			const gchar *cmd[] = {"seek", "-10", NULL};

			mpv_command(inst->gmpv_ctx->mpv_ctx, cmd);
		}
	}
}

static void proxy_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	media_keys *inst = data;

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
				inst );
}

static void session_ready_handler(	GObject *source_object,
					GAsyncResult *res,
					gpointer data )
{
	media_keys *inst = data;

	inst->session_bus_conn = g_bus_get_finish(res, NULL);

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

void media_keys_init(gmpv_handle *gmpv_ctx)
{
	media_keys *inst = g_malloc(sizeof(media_keys));

	inst->gmpv_ctx = gmpv_ctx;
	inst->shutdown_sig_id = 0;
	inst->proxy = NULL;
	inst->session_bus_conn = NULL;

	inst->shutdown_sig_id
		= g_signal_connect(	inst->gmpv_ctx->gui,
					"delete-event",
					G_CALLBACK(delete_handler),
					inst );

	g_bus_get(G_BUS_TYPE_SESSION, NULL, session_ready_handler, inst);
}
