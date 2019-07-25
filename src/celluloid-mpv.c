/*
 * Copyright (c) 2014-2019 gnome-mpv
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
#include <gtk/gtk.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>

#include <epoxy/gl.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <epoxy/glx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#include <epoxy/egl.h>
#endif
#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>
#include <epoxy/wgl.h>
#endif

#include "celluloid-mpv.h"
#include "celluloid-mpv-private.h"
#include "celluloid-mpv-wrapper.h"
#include "celluloid-common.h"
#include "celluloid-def.h"
#include "celluloid-marshal.h"

static void *
get_proc_address(void *fn_ctx, const gchar *name);

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

static
void dispose(GObject *object);

static
void finalize(GObject *object);

static
void wakeup_callback(void *data);

static void
mpv_property_changed(CelluloidMpv *mpv, const gchar *name, gpointer value);

static void
mpv_log_message(	CelluloidMpv *mpv,
			mpv_log_level log_level,
			const gchar *prefix,
			const gchar *text );

static void
mpv_event_notify(CelluloidMpv *mpv, gint event_id, gpointer event_data);

static gboolean
process_mpv_events(gpointer data);

static gboolean
check_mpv_version(const gchar *version);

static gpointer
get_wl_display(void);

static gpointer
get_x11_display(void);

static void
initialize(CelluloidMpv *mpv);

static void
load_file(CelluloidMpv *mpv, const gchar *uri, gboolean append);

static void
reset(CelluloidMpv *mpv);

G_DEFINE_TYPE_WITH_PRIVATE(CelluloidMpv, celluloid_mpv, G_TYPE_OBJECT)

static void *
get_proc_address(void *fn_ctx, const gchar *name)
{
	GdkDisplay *display = gdk_display_get_default();

#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY(display))
		return eglGetProcAddress(name);
#endif
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(display))
		return	(void *)(intptr_t)
			glXGetProcAddressARB((const GLubyte *)name);
#endif
#ifdef GDK_WINDOWING_WIN32
	if (GDK_IS_WIN32_DISPLAY(display))
		return wglGetProcAddress(name);
#endif
	g_assert_not_reached();
	return NULL;
}

static void
set_property(	GObject *object,
		guint property_id,
		const GValue *value,
		GParamSpec *pspec )
{
	CelluloidMpvPrivate *priv = get_private(CELLULOID_MPV(object));

	if(property_id == PROP_WID)
	{
		priv->wid = g_value_get_int64(value);
	}
	else if(property_id == PROP_READY)
	{
		priv->ready = g_value_get_boolean(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
get_property(	GObject *object,
		guint property_id,
		GValue *value,
		GParamSpec *pspec )
{
	CelluloidMpvPrivate *priv = get_private(CELLULOID_MPV(object));

	if(property_id == PROP_WID)
	{
		g_value_set_int64(value, priv->wid);
	}
	else if(property_id == PROP_READY)
	{
		g_value_set_boolean(value, priv->ready);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
dispose(GObject *object)
{
	CelluloidMpv *mpv = CELLULOID_MPV(object);

	if(get_private(mpv)->mpv_ctx)
	{
		celluloid_mpv_quit(mpv);
		while(g_source_remove_by_user_data(object));
	}

	G_OBJECT_CLASS(celluloid_mpv_parent_class)->dispose(object);
}

static void
finalize(GObject *object)
{
	G_OBJECT_CLASS(celluloid_mpv_parent_class)->finalize(object);
}

static void
wakeup_callback(void *data)
{
	g_idle_add_full(G_PRIORITY_HIGH_IDLE, process_mpv_events, data, NULL);
}

static void
mpv_property_changed(CelluloidMpv *mpv, const gchar *name, gpointer value)
{
	g_debug("Received mpv property change event for \"%s\"", name);
}

static void
mpv_event_notify(CelluloidMpv *mpv, gint event_id, gpointer event_data)
{
	if(event_id == MPV_EVENT_PROPERTY_CHANGE)
	{
		mpv_event_property *prop = event_data;

		g_signal_emit_by_name(	mpv,
					"mpv-property-changed",
					prop->name,
					prop->data );
	}
	else if(event_id == MPV_EVENT_IDLE)
	{
		celluloid_mpv_set_property_flag(mpv, "pause", TRUE);
	}
	else if(event_id == MPV_EVENT_END_FILE)
	{
		GSettings *settings = g_settings_new(CONFIG_ROOT);
		const gchar *ignore_key = "ignore-playback-errors";

		mpv_event_end_file *ef_event = event_data;

		if(	!g_settings_get_boolean(settings, ignore_key) &&
			ef_event->reason == MPV_END_FILE_REASON_ERROR )
		{
			const gchar *err;
			gchar *msg;

			err = mpv_error_string(ef_event->error);
			msg = g_strdup_printf
				(	_("Playback was terminated "
					"abnormally. Reason: %s."),
					err );

			celluloid_mpv_set_property_flag(mpv, "pause", TRUE);
			g_signal_emit_by_name(mpv, "error", msg);

			g_free(msg);
		}

		g_object_unref(settings);
	}
	else if(event_id == MPV_EVENT_LOG_MESSAGE)
	{
		mpv_event_log_message* message = event_data;

		g_signal_emit_by_name(	mpv,
					"mpv-log-message",
					message->log_level,
					message->prefix,
					message->text );
	}
	else if(event_id == MPV_EVENT_CLIENT_MESSAGE)
	{
		mpv_event_client_message *event_cmsg = event_data;
		gchar* msg = strnjoinv(	" ",
					event_cmsg->args,
					(gsize)event_cmsg->num_args );

		g_signal_emit_by_name(mpv, "message", msg);
		g_free(msg);
	}
	else if(event_id == MPV_EVENT_SHUTDOWN)
	{
		g_signal_emit_by_name(mpv, "shutdown");
	}
}

static gboolean
process_mpv_events(gpointer data)
{
	CelluloidMpv *mpv = data;
	CelluloidMpvPrivate *priv = get_private(mpv);
	gboolean done = !mpv;

	while(!done)
	{
		mpv_event *event =	priv->mpv_ctx?
					mpv_wait_event(priv->mpv_ctx, 0):
					NULL;

		if(event)
		{
			if(	!priv->mpv_ctx ||
				event->event_id == MPV_EVENT_SHUTDOWN ||
				event->event_id == MPV_EVENT_NONE )
			{
				done = TRUE;
			}

			g_signal_emit_by_name(	mpv,
						"mpv-event-notify",
						event->event_id,
						event->data );
		}
		else
		{
			done = TRUE;
		}
	}

	return FALSE;
}

static gboolean
check_mpv_version(const gchar *version)
{
	guint64 min_version[] = {MIN_MPV_MAJOR, MIN_MPV_MINOR, MIN_MPV_PATCH};
	const guint min_version_length = G_N_ELEMENTS(min_version);
	gchar **tokens = NULL;
	gboolean done = FALSE;
	gboolean result = TRUE;

	/* Skip to the version number */
	if(strncmp(version, "mpv ", 4) == 0)
	{
		tokens = g_strsplit(version+4, ".", (gint)min_version_length);
	}

	done = !tokens || g_strv_length(tokens) != min_version_length;
	result = !done;

	for(guint i = 0; i < min_version_length && !done && result; i++)
	{
		gchar *endptr = NULL;
		guint64 token = g_ascii_strtoull(tokens[i], &endptr, 10);

		/* If the token is equal to the minimum, continue checking the
		 * rest of the tokens. If it is greater, just skip them.
		 */
		result &= !(*endptr) && (token >= min_version[i]);
		done = result && (token >= min_version[i]);
	}

	g_strfreev(tokens);

	return result;
}

static gpointer
get_wl_display(void)
{
	gpointer wl_display = NULL;

#ifdef GDK_WINDOWING_WAYLAND
	GdkDisplay *display = gdk_display_get_default();

	if(GDK_IS_WAYLAND_DISPLAY(display))
	{
		wl_display =  gdk_wayland_display_get_wl_display(display);
	}
#endif

	return wl_display;
}

static gpointer
get_x11_display(void)
{
	gpointer x11_display = NULL;

#ifdef GDK_WINDOWING_X11
	GdkDisplay *display = gdk_display_get_default();

	if(GDK_IS_X11_DISPLAY(display))
	{
		x11_display = gdk_x11_display_get_xdisplay(display);
	}
#endif

	return x11_display;
}

static void
initialize(CelluloidMpv *mpv)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
	gchar *current_vo = NULL;
	gchar *mpv_version = NULL;

	if(priv->wid < 0)
	{
		g_info("Forcing --vo=libmpv");
		mpv_set_option_string(priv->mpv_ctx, "vo", "libmpv");
	}
	else if(priv->wid == 0)
	{
		g_info("Forcing --vo=null");
		mpv_set_option_string(priv->mpv_ctx, "vo", "null");
	}
	else
	{
		g_debug("Attaching mpv window to wid %#x", (guint)priv->wid);
		mpv_set_option(priv->mpv_ctx, "wid", MPV_FORMAT_INT64, &priv->wid);
	}

	mpv_set_wakeup_callback(priv->mpv_ctx, wakeup_callback, mpv);
	mpv_initialize(priv->mpv_ctx);

	mpv_version = celluloid_mpv_get_property_string(mpv, "mpv-version");
	current_vo = celluloid_mpv_get_property_string(mpv, "current-vo");
	priv->use_opengl = (!current_vo && priv->wid != 0);

	g_info("Using %s", mpv_version);

	if(!check_mpv_version(mpv_version))
	{
		g_warning(	"Minimum mpv version requirement (%d.%d.%d) not met",
				MIN_MPV_MAJOR,
				MIN_MPV_MINOR,
				MIN_MPV_PATCH );
	}

	priv->ready = TRUE;
	g_object_notify(G_OBJECT(mpv), "ready");

	mpv_free(current_vo);
	mpv_free(mpv_version);
}

static void
load_file(CelluloidMpv *mpv, const gchar *uri, gboolean append)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
	gchar *path = get_path_from_uri(uri);
	const gchar *load_cmd[] = {"loadfile", path, NULL, NULL};
	gint64 playlist_count = 0;

	g_assert(uri);
	g_info(	"Loading file (append=%s): %s", append?"TRUE":"FALSE", uri);

	mpv_get_property(	priv->mpv_ctx,
				"playlist-count",
				MPV_FORMAT_INT64,
				&playlist_count );

	load_cmd[2] = (append && playlist_count > 0)?"append":"replace";

	if(!append)
	{
		celluloid_mpv_set_property_flag(mpv, "pause", FALSE);
	}

	g_assert(priv->mpv_ctx);
	mpv_request_event(priv->mpv_ctx, MPV_EVENT_END_FILE, 0);
	mpv_command(priv->mpv_ctx, load_cmd);
	mpv_request_event(priv->mpv_ctx, MPV_EVENT_END_FILE, 1);

	g_free(path);
}

static void
reset(CelluloidMpv *mpv)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
	gchar *loop_file_str;
	gchar *loop_playlist_str;
	gboolean loop_file;
	gboolean loop_playlist;

	loop_file_str =		celluloid_mpv_get_property_string
				(mpv, "loop-file");
	loop_playlist_str =	celluloid_mpv_get_property_string
				(mpv, "loop-playlist");
	loop_file =		(g_strcmp0(loop_file_str, "inf") == 0);
	loop_playlist =		(g_strcmp0(loop_playlist_str, "inf") == 0);

	mpv_free(loop_file_str);
	mpv_free(loop_playlist_str);

	/* Reset priv->mpv_ctx */
	priv->ready = FALSE;
	g_object_notify(G_OBJECT(mpv), "ready");

	celluloid_mpv_command_string(mpv, "write-watch-later-config");
	celluloid_mpv_quit(mpv);

	priv->mpv_ctx = mpv_create();
	celluloid_mpv_initialize(mpv);

	celluloid_mpv_set_render_update_callback
		(	mpv,
			priv->render_update_callback,
			priv->render_update_callback_data );

	celluloid_mpv_set_property_string
		(mpv, "loop-file", loop_file?"inf":"no");
	celluloid_mpv_set_property_string
		(mpv, "loop-playlist", loop_playlist?"inf":"no");
}

static void
mpv_log_message(	CelluloidMpv *mpv,
			mpv_log_level log_level,
			const gchar *prefix,
			const gchar *text )
{
}

static void
celluloid_mpv_class_init(CelluloidMpvClass* klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	klass->mpv_event_notify = mpv_event_notify;
	klass->mpv_log_message = mpv_log_message;
	klass->mpv_property_changed = mpv_property_changed;
	klass->initialize = initialize;
	klass->load_file = load_file;
	klass->reset = reset;
	obj_class->set_property = set_property;
	obj_class->get_property = get_property;
	obj_class->dispose = dispose;
	obj_class->finalize = finalize;

	pspec = g_param_spec_int64
		(	"wid",
			"WID",
			"The ID of the window to attach to",
			G_MININT64,
			G_MAXINT64,
			-1,
			G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_WID, pspec);

	pspec = g_param_spec_boolean
		(	"ready",
			"Ready",
			"Whether mpv is initialized and ready to receive commands",
			FALSE,
			G_PARAM_READABLE );
	g_object_class_install_property(obj_class, PROP_READY, pspec);

	g_signal_new(	"error",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
	g_signal_new(	"mpv-event-notify",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(CelluloidMpvClass, mpv_event_notify),
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT_POINTER,
			G_TYPE_NONE,
			2,
			G_TYPE_INT,
			G_TYPE_POINTER );
	g_signal_new(	"mpv-log-message",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(CelluloidMpvClass, mpv_log_message),
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT_STRING_STRING,
			G_TYPE_NONE,
			3,
			G_TYPE_INT,
			G_TYPE_STRING,
			G_TYPE_STRING );
	g_signal_new(	"mpv-property-changed",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(CelluloidMpvClass, mpv_property_changed),
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__STRING_POINTER,
			G_TYPE_NONE,
			2,
			G_TYPE_STRING,
			G_TYPE_POINTER );
	g_signal_new(	"message",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
	g_signal_new(	"window-resize",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__INT64_INT64,
			G_TYPE_NONE,
			2,
			G_TYPE_INT64,
			G_TYPE_INT64 );
	g_signal_new(	"window-move",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_gen_marshal_VOID__BOOLEAN_BOOLEAN_POINTER_POINTER,
			G_TYPE_NONE,
			4,
			G_TYPE_BOOLEAN,
			G_TYPE_BOOLEAN,
			G_TYPE_POINTER,
			G_TYPE_POINTER );
	g_signal_new(	"shutdown",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
}

static void
celluloid_mpv_init(CelluloidMpv *mpv)
{
	CelluloidMpvPrivate *priv = get_private(mpv);

	setlocale(LC_NUMERIC, "C");

	priv->mpv_ctx = mpv_create();
	priv->render_ctx = NULL;
	priv->ready = FALSE;
	priv->init_vo_config = TRUE;
	priv->use_opengl = FALSE;
	priv->wid = -1;
	priv->render_update_callback_data = NULL;
	priv->render_update_callback = NULL;
}

CelluloidMpv *
celluloid_mpv_new(gint64 wid)
{
	return CELLULOID_MPV(g_object_new(celluloid_mpv_get_type(), "wid", wid, NULL));
}

inline mpv_render_context *
celluloid_mpv_get_render_context(CelluloidMpv *mpv)
{
	return get_private(mpv)->render_ctx;
}

inline gboolean
celluloid_mpv_get_use_opengl_cb(CelluloidMpv *mpv)
{
	return get_private(mpv)->use_opengl;
}

void
celluloid_mpv_initialize(CelluloidMpv *mpv)
{
	CELLULOID_MPV_GET_CLASS(mpv)->initialize(mpv);
}

void
celluloid_mpv_init_gl(CelluloidMpv *mpv)
{
	CelluloidMpvPrivate *priv = get_private(mpv);
	mpv_opengl_init_params init_params =
		{.get_proc_address = get_proc_address};
	mpv_render_param params[] =
		{	{MPV_RENDER_PARAM_API_TYPE, MPV_RENDER_API_TYPE_OPENGL},
			{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &init_params},
			{MPV_RENDER_PARAM_WL_DISPLAY, get_wl_display()},
			{MPV_RENDER_PARAM_X11_DISPLAY, get_x11_display()},
			{0, NULL} };
	gint rc = mpv_render_context_create(	&priv->render_ctx,
						priv->mpv_ctx,
						params );

	if(rc >= 0)
	{
		g_debug("Initialized render context");
	}
	else
	{
		g_critical("Failed to initialize render context");
	}
}

void
celluloid_mpv_reset(CelluloidMpv *mpv)
{
	CELLULOID_MPV_GET_CLASS(mpv)->reset(mpv);
}

void
celluloid_mpv_quit(CelluloidMpv *mpv)
{
	CelluloidMpvPrivate *priv = get_private(mpv);

	g_info("Terminating mpv");
	celluloid_mpv_command_string(mpv, "quit");

	if(priv->render_ctx)
	{
		g_debug("Uninitializing render context");
		mpv_render_context_free(priv->render_ctx);

		priv->render_ctx = NULL;
	}

	g_assert(priv->mpv_ctx);
	mpv_terminate_destroy(priv->mpv_ctx);

	priv->mpv_ctx = NULL;
}

void
celluloid_mpv_load_track(CelluloidMpv *mpv, const gchar *uri, TrackType type)
{
	const gchar *cmd[3] = {NULL};
	gchar *path = g_filename_from_uri(uri, NULL, NULL);

	if(type == TRACK_TYPE_AUDIO)
	{
		cmd[0] = "audio-add";
	}
	else if(type == TRACK_TYPE_SUBTITLE)
	{
		cmd[0] = "sub-add";
	}
	else
	{
		g_assert_not_reached();
	}

	cmd[1] = path?:uri;

	g_debug("Loading external track %s with type %d", cmd[1], type);
	celluloid_mpv_command(mpv, cmd);

	g_free(path);
}

void
celluloid_mpv_load_file(CelluloidMpv *mpv, const gchar *uri, gboolean append)
{
	CELLULOID_MPV_GET_CLASS(mpv)->load_file(mpv, uri, append);
}

void
celluloid_mpv_load(CelluloidMpv *mpv, const gchar *uri, gboolean append)
{
	const gchar *subtitle_exts[] = SUBTITLE_EXTS;

	if(extension_matches(uri, subtitle_exts))
	{
		celluloid_mpv_load_track(mpv, uri, TRACK_TYPE_SUBTITLE);
	}
	else
	{
		celluloid_mpv_load_file(mpv, uri, append);
	}
}
