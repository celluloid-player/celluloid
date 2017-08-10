/*
 * Copyright (c) 2014-2017 gnome-mpv
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
#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
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

#include "gmpv_mpv.h"
#include "gmpv_mpv_private.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_mpv_options.h"
#include "gmpv_common.h"
#include "gmpv_def.h"
#include "gmpv_marshal.h"

static void *GLAPIENTRY glMPGetNativeDisplay(const gchar *name);
static void *get_proc_address(void *fn_ctx, const gchar *name);
static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void dispose(GObject *object);
static void finalize(GObject *object);
static void apply_default_options(GmpvMpv *mpv);
static void load_mpv_config_file(GmpvMpv *mpv);
static void apply_extra_options(GmpvMpv *mpv);
static void load_input_config_file(GmpvMpv *mpv);
static void observe_properties(GmpvMpv *mpv);
static void load_from_playlist(GmpvMpv *mpv);
static void load_scripts(GmpvMpv *mpv);
static void wakeup_callback(void *data);
static GmpvPlaylistEntry *parse_playlist_entry(mpv_node_list *node);
static GmpvTrack *parse_track_entry(mpv_node_list *node);
static void mpv_prop_change_handler(GmpvMpv *mpv, mpv_event_property* prop);
static gboolean mpv_event_handler(gpointer data);
static gint apply_args(mpv_handle *mpv_ctx, gchar *args);
static void log_handler(GmpvMpv *mpv, mpv_event_log_message* message);
static void load_input_conf(GmpvMpv *mpv, const gchar *input_conf);
static void add_file_to_playlist(GmpvMpv *mpv, const gchar *uri);
static void update_playlist(GmpvMpv *mpv);
static void update_metadata(GmpvMpv *mpv);
static void update_track_list(GmpvMpv *mpv);

G_DEFINE_TYPE_WITH_PRIVATE(GmpvMpv, gmpv_mpv, G_TYPE_OBJECT)

static void *GLAPIENTRY glMPGetNativeDisplay(const gchar *name)
{
       GdkDisplay *display = gdk_display_get_default();

#ifdef GDK_WINDOWING_WAYLAND
       if(GDK_IS_WAYLAND_DISPLAY(display) && g_strcmp0(name, "wl") == 0)
               return gdk_wayland_display_get_wl_display(display);
#endif
#ifdef GDK_WINDOWING_X11
       if(GDK_IS_X11_DISPLAY(display) && g_strcmp0(name, "x11") == 0)
               return gdk_x11_display_get_xdisplay(display);
#endif

       return NULL;
}

static void *get_proc_address(void *fn_ctx, const gchar *name)
{
	GdkDisplay *display = gdk_display_get_default();

	if(g_strcmp0(name, "glMPGetNativeDisplay") == 0)
		return glMPGetNativeDisplay;

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
}

static void set_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMpvPrivate *priv = get_private(GMPV_MPV(object));

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

static void get_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvMpvPrivate *priv = get_private(GMPV_MPV(object));

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

static void dispose(GObject *object)
{
	GmpvMpv *mpv = GMPV_MPV(object);

	if(get_private(mpv)->mpv_ctx)
	{
		gmpv_mpv_quit(mpv);
		while(g_source_remove_by_user_data(object));
	}

	G_OBJECT_CLASS(gmpv_mpv_parent_class)->dispose(object);
}

static void finalize(GObject *object)
{
	GmpvMpvPrivate *priv = get_private(GMPV_MPV(object));

	g_ptr_array_free(priv->playlist, TRUE);
	g_ptr_array_free(priv->metadata, TRUE);
	g_ptr_array_free(priv->track_list, TRUE);
	g_free(priv->tmp_input_file);

	g_slist_free_full(	priv->log_level_list,
				(GDestroyNotify)module_log_level_free);

	G_OBJECT_CLASS(gmpv_mpv_parent_class)->finalize(object);
}

static void apply_default_options(GmpvMpv *mpv)
{
	gchar *config_dir = get_config_dir_path();
	gchar *watch_dir = get_watch_dir_path();

	const struct
	{
		const gchar *name;
		const gchar *value;
	}
	options[] = {	{"vo", "opengl,vdpau,vaapi,xv,x11,opengl-cb,"},
			{"osd-level", "1"},
			{"softvol", "yes"},
			{"force-window", "immediate"},
			{"input-default-bindings", "yes"},
			{"audio-client-name", ICON_NAME},
			{"title", "${media-title}"},
			{"autofit-larger", "75%"},
			{"window-scale", "1"},
			{"pause", "yes"},
			{"ytdl", "yes"},
			{"load-scripts", "no"},
			{"osd-bar", "no"},
			{"input-cursor", "no"},
			{"cursor-autohide", "no"},
			{"softvol-max", "100"},
			{"config", "no"},
			{"config-dir", config_dir},
			{"watch-later-directory", watch_dir},
			{"screenshot-template", "gnome-mpv-shot%n"},
			{NULL, NULL} };

	for(gint i = 0; options[i].name; i++)
	{
		g_debug(	"Applying default option --%s=%s",
				options[i].name,
				options[i].value );

		mpv_set_option_string(	get_private(mpv)->mpv_ctx,
					options[i].name,
					options[i].value );
	}

	g_free(config_dir);
	g_free(watch_dir);
}

static void load_mpv_config_file(GmpvMpv *mpv)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	if(g_settings_get_boolean(settings, "mpv-config-enable"))
	{
		gchar *mpv_conf =	g_settings_get_string
					(settings, "mpv-config-file");

		g_info("Loading config file: %s", mpv_conf);
		mpv_load_config_file(get_private(mpv)->mpv_ctx, mpv_conf);

		g_free(mpv_conf);
	}

	g_object_unref(settings);
}

static void apply_extra_options(GmpvMpv *mpv)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gchar *extra_options = g_settings_get_string(settings, "mpv-options");

	g_debug("Applying extra mpv options: %s", extra_options);

	/* Apply extra options */
	if(apply_args(get_private(mpv)->mpv_ctx, extra_options) < 0)
	{
		const gchar *msg = _("Failed to apply one or more MPV options.");
		g_signal_emit_by_name(mpv, "error", msg);
	}

	g_free(extra_options);
	g_object_unref(settings);
}

static void load_input_config_file(GmpvMpv *mpv)
{
	GSettings *settings = g_settings_new(CONFIG_ROOT);

	if(g_settings_get_boolean(settings, "mpv-input-config-enable"))
	{
		gchar *input_conf =	g_settings_get_string
					(settings, "mpv-input-config-file");

		g_info("Loading input config file: %s", input_conf);
		load_input_conf(mpv, input_conf);

		g_free(input_conf);
	}
	else
	{
		load_input_conf(mpv, NULL);
	}

	g_object_unref(settings);
}

static void observe_properties(GmpvMpv *mpv)
{
	mpv_handle *mpv_ctx = get_private(mpv)->mpv_ctx;

	mpv_observe_property(mpv_ctx, 0, "aid", MPV_FORMAT_STRING);
	mpv_observe_property(mpv_ctx, 0, "vid", MPV_FORMAT_STRING);
	mpv_observe_property(mpv_ctx, 0, "sid", MPV_FORMAT_STRING);
	mpv_observe_property(mpv_ctx, 0, "chapters", MPV_FORMAT_INT64);
	mpv_observe_property(mpv_ctx, 0, "core-idle", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv_ctx, 0, "idle-active", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv_ctx, 0, "fullscreen", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv_ctx, 0, "pause", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv_ctx, 0, "loop", MPV_FORMAT_STRING);
	mpv_observe_property(mpv_ctx, 0, "duration", MPV_FORMAT_DOUBLE);
	mpv_observe_property(mpv_ctx, 0, "media-title", MPV_FORMAT_STRING);
	mpv_observe_property(mpv_ctx, 0, "metadata", MPV_FORMAT_NODE);
	mpv_observe_property(mpv_ctx, 0, "playlist", MPV_FORMAT_NODE);
	mpv_observe_property(mpv_ctx, 0, "playlist-count", MPV_FORMAT_INT64);
	mpv_observe_property(mpv_ctx, 0, "playlist-pos", MPV_FORMAT_INT64);
	mpv_observe_property(mpv_ctx, 0, "speed", MPV_FORMAT_DOUBLE);
	mpv_observe_property(mpv_ctx, 0, "track-list", MPV_FORMAT_NODE);
	mpv_observe_property(mpv_ctx, 0, "vo-configured", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv_ctx, 0, "volume", MPV_FORMAT_DOUBLE);
}

static void load_from_playlist(GmpvMpv *mpv)
{
	GPtrArray * playlist = get_private(mpv)->playlist;

	for(guint i = 0; playlist && i < playlist->len; i++)
	{
		GmpvPlaylistEntry *entry = g_ptr_array_index(playlist, i);

		/* Do not append on first iteration */
		gmpv_mpv_load_file(mpv, entry->filename, i != 0);
	}
}

static void load_scripts(GmpvMpv *mpv)
{
	gchar *path = get_scripts_dir_path();
	GDir *dir = g_dir_open(path, 0, NULL);

	if(dir)
	{
		const gchar *name;

		do
		{
			gchar *full_path;

			name = g_dir_read_name(dir);
			full_path = g_build_filename(path, name, NULL);

			if(g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
			{
				const gchar *cmd[]
					= {"load-script", full_path, NULL};

				g_info("Loading script: %s", full_path);
				mpv_command(get_private(mpv)->mpv_ctx, cmd);
			}

			g_free(full_path);
		}
		while(name);

		g_dir_close(dir);
	}
	else
	{
		g_warning("Failed to open scripts directory: %s", path);
	}

	g_free(path);
}

static void wakeup_callback(void *data)
{
	g_idle_add_full(G_PRIORITY_HIGH_IDLE, mpv_event_handler, data, NULL);
}

static GmpvPlaylistEntry *parse_playlist_entry(mpv_node_list *node)
{
	const gchar *filename = NULL;
	const gchar *title = NULL;

	for(gint i = 0; i < node->num; i++)
	{
		if(g_strcmp0(node->keys[i], "filename") == 0)
		{
			filename = node->values[i].u.string;
		}
		else if(g_strcmp0(node->keys[i], "title") == 0)
		{
			title = node->values[i].u.string;
		}
	}

	return gmpv_playlist_entry_new(filename, title);
}

static GmpvTrack *parse_track_entry(mpv_node_list *node)
{
	GmpvTrack *entry = gmpv_track_new();

	for(gint i = 0; i < node->num; i++)
	{
		if(g_strcmp0(node->keys[i], "type") == 0)
		{
			const gchar *type = node->values[i].u.string;

			if(g_strcmp0(type, "audio") == 0)
			{
				entry->type = TRACK_TYPE_AUDIO;
			}
			else if(g_strcmp0(type, "video") == 0)
			{
				entry->type = TRACK_TYPE_VIDEO;
			}
			else if(g_strcmp0(type, "sub") == 0)
			{
				entry->type = TRACK_TYPE_SUBTITLE;
			}
		}
		else if(g_strcmp0(node->keys[i], "title") == 0)
		{
			entry->title = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "lang") == 0)
		{
			entry->lang = g_strdup(node->values[i].u.string);
		}
		else if(g_strcmp0(node->keys[i], "id") == 0)
		{
			entry->id = node->values[i].u.int64;
		}
	}

	return entry;
}

static void mpv_prop_change_handler(GmpvMpv *mpv, mpv_event_property* prop)
{
	GmpvMpvPrivate *priv = get_private(mpv);

	g_debug("Received mpv property change event for \"%s\"", prop->name);

	if(g_strcmp0(prop->name, "pause") == 0)
	{
		gboolean idle_active = FALSE;
		gboolean pause = prop->data?*((int *)prop->data):TRUE;

		mpv_get_property(	priv->mpv_ctx,
					"idle-active",
					MPV_FORMAT_FLAG,
					&idle_active );

		if(idle_active && !pause && !priv->init_vo_config)
		{
			load_from_playlist(mpv);
		}
	}
	else if(g_strcmp0(prop->name, "playlist") == 0)
	{
		gint64 playlist_count = 0;
		gboolean idle_active = FALSE;
		gboolean was_empty = FALSE;

		mpv_get_property(	priv->mpv_ctx,
					"playlist-count",
					MPV_FORMAT_INT64,
					&playlist_count );
		mpv_get_property(	priv->mpv_ctx,
					"idle-active",
					MPV_FORMAT_FLAG,
					&idle_active );

		was_empty = (priv->playlist->len == 0);

		if(!idle_active)
		{
			update_playlist(mpv);
		}

		/* Check if we're transitioning from empty playlist to non-empty
		 * playlist.
		 */
		if(was_empty && priv->playlist->len > 0)
		{
			gmpv_mpv_set_property_flag(mpv, "pause", FALSE);
		}
	}
	else if(g_strcmp0(prop->name, "metadata") == 0)
	{
		update_metadata(mpv);
	}
	else if(g_strcmp0(prop->name, "track-list") == 0)
	{
		update_track_list(mpv);
	}
	else if(g_strcmp0(prop->name, "vo-configured") == 0)
	{
		if(priv->init_vo_config)
		{
			priv->init_vo_config = FALSE;
			load_from_playlist(mpv);
			load_scripts(mpv);
		}
	}
}

static gboolean mpv_event_handler(gpointer data)
{
	GmpvMpv *mpv = data;
	GmpvMpvPrivate *priv = get_private(mpv);
	gboolean done = !mpv;

	while(!done)
	{
		mpv_event *event =	priv->mpv_ctx?
					mpv_wait_event(priv->mpv_ctx, 0):
					NULL;

		if(!event)
		{
			done = TRUE;
		}
		else if(event->event_id == MPV_EVENT_PROPERTY_CHANGE)
		{
			mpv_event_property *prop = event->data;

			mpv_prop_change_handler(mpv, prop);

			g_signal_emit_by_name(	mpv,
						"mpv-prop-change",
						prop->name,
						prop->data );
		}
		else if(event->event_id == MPV_EVENT_IDLE)
		{
			priv->loaded = FALSE;
			gmpv_mpv_set_property_flag(mpv, "pause", TRUE);
		}
		else if(event->event_id == MPV_EVENT_FILE_LOADED)
		{
			priv->loaded = TRUE;
		}
		else if(event->event_id == MPV_EVENT_END_FILE)
		{
			mpv_event_end_file *ef_event = event->data;

			if(priv->loaded)
			{
				priv->new_file = FALSE;
			}

			if(ef_event->reason == MPV_END_FILE_REASON_ERROR)
			{
				const gchar *err;
				gchar *msg;

				err = mpv_error_string(ef_event->error);
				msg = g_strdup_printf
					(	_("Playback was terminated "
						"abnormally. Reason: %s."),
						err );

				gmpv_mpv_set_property_flag(mpv, "pause", TRUE);
				g_signal_emit_by_name(mpv, "error", msg);

				g_free(msg);
			}
		}
		else if(event->event_id == MPV_EVENT_VIDEO_RECONFIG)
		{
			g_signal_emit_by_name(mpv, "mpv-video-reconfig");
		}
		else if(event->event_id == MPV_EVENT_START_FILE)
		{
			gboolean vo_configured = FALSE;

			mpv_get_property(	priv->mpv_ctx,
						"vo-configured",
						MPV_FORMAT_FLAG,
						&vo_configured );

			/* If the vo is not configured yet, save the content of
			 * mpv's playlist in priv->playlist. This will be loaded
			 * again when the vo is configured.
			 */
			if(!vo_configured)
			{
				update_playlist(mpv);
			}
		}
		else if(event->event_id == MPV_EVENT_PLAYBACK_RESTART)
		{
			g_signal_emit_by_name(mpv, "mpv-playback-restart");
		}
		else if(event->event_id == MPV_EVENT_LOG_MESSAGE)
		{
			log_handler(mpv, event->data);
		}
		else if(event->event_id == MPV_EVENT_CLIENT_MESSAGE)
		{
			mpv_event_client_message *event_cmsg = event->data;
			gchar* msg = strnjoinv(	" ",
						event_cmsg->args,
						(gsize)event_cmsg->num_args );

			g_signal_emit_by_name(mpv, "message", msg);
			g_free(msg);
		}
		else if(event->event_id == MPV_EVENT_SHUTDOWN)
		{
			g_signal_emit_by_name(mpv, "shutdown");

			done = TRUE;
		}
		else if(event->event_id == MPV_EVENT_NONE)
		{
			done = TRUE;
		}

		if(event && !priv->mpv_ctx)
		{
			done = TRUE;
		}
	}

	return FALSE;
}

static gint apply_args(mpv_handle *mpv_ctx, gchar *args)
{
	gint fail_count = 0;
	gchar **tokens = g_regex_split_simple(	"(^|\\s+)--",
						args,
						G_REGEX_NO_AUTO_CAPTURE,
						0 );

	/* Skip the first token if it's non-NULL, since it is just going to be
	 * empty string for any valid args.
	 */
	for(gint i = tokens[0]?1:0; tokens[i]; i++)
	{
		gchar **parts = g_strsplit(g_strchomp(tokens[i]), "=", 2);
		const gchar *option = parts[0];
		const gchar *value = (option?parts[1]:NULL)?:"";

		g_debug("Applying option: --%s", tokens[i]);

		if(mpv_set_option_string(mpv_ctx, option, value) < 0)
		{
			fail_count++;

			g_warning("Failed to apply option: --%s\n", tokens[i]);
		}

		g_strfreev(parts);
	}

	g_strfreev(tokens);

	return fail_count*(-1);
}

static void log_handler(GmpvMpv *mpv, mpv_event_log_message* message)
{
	GSList *iter = get_private(mpv)->log_level_list;
	module_log_level *level = iter?iter->data:NULL;
	gsize event_prefix_len = strlen(message->prefix);
	gboolean found = FALSE;

	while(iter && !found)
	{
		gsize prefix_len = strlen(level->prefix);
		gint cmp = strncmp(	level->prefix,
					message->prefix,
					(event_prefix_len < prefix_len)?
					event_prefix_len:prefix_len );

		/* Allow both exact match and prefix match */
		if(cmp == 0
		&& (prefix_len == event_prefix_len
		|| (prefix_len < event_prefix_len
		&& message->prefix[prefix_len] == '/')))
		{
			found = TRUE;
		}
		else
		{
			iter = g_slist_next(iter);
			level = iter?iter->data:NULL;
		}
	}

	if(!iter || (message->log_level <= level->level))
	{
		gchar *buf = g_strdup(message->text);
		gsize len = strlen(buf);

		if(len > 1)
		{
			/* g_message() automatically adds a newline
			 * character when using the default log handler,
			 * but log messages from mpv already come
			 * terminated with a newline character so we
			 * need to take it out.
			 */
			if(buf[len-1] == '\n')
			{
				buf[len-1] = '\0';
			}

			g_message("[%s] %s", message->prefix, buf);
		}

		g_free(buf);
	}
}

static void load_input_conf(GmpvMpv *mpv, const gchar *input_conf)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	const gchar *default_keybinds[] = DEFAULT_KEYBINDS;
	gchar *tmp_path;
	FILE *tmp_file;
	gint tmp_fd;

	tmp_fd = g_file_open_tmp(NULL, &tmp_path, NULL);
	tmp_file = fdopen(tmp_fd, "w");
	priv->tmp_input_file = tmp_path;

	if(!tmp_file)
	{
		g_error("Failed to open temporary input config file");
	}

	for(gint i = 0; default_keybinds[i]; i++)
	{
		const gsize len = strlen(default_keybinds[i]);
		gsize write_size = fwrite(default_keybinds[i], len, 1, tmp_file);
		gint rc = fputc('\n', tmp_file);

		if(write_size != 1 || rc != '\n')
		{
			g_error(	"Error writing default keybindings to "
					"temporary input config file" );
		}
	}

	g_debug("Using temporary input config file: %s", tmp_path);
	mpv_set_option_string(priv->mpv_ctx, "input-conf", tmp_path);

	if(input_conf && strlen(input_conf) > 0)
	{
		const gsize buf_size = 65536;
		void *buf = g_malloc(buf_size);
		FILE *src_file = g_fopen(input_conf, "r");
		gsize read_size = buf_size;

		if(!src_file)
		{
			g_warning(	"Cannot open input config file: %s",
					input_conf );
		}

		while(src_file && read_size == buf_size)
		{
			read_size = fread(buf, 1, buf_size, src_file);

			if(read_size != fwrite(buf, 1, read_size, tmp_file))
			{
				g_error(	"Error writing requested input "
						"config file to temporary "
						"input config file" );
			}
		}

		g_info("Loaded input config file: %s", input_conf);

		g_free(buf);
	}

	fclose(tmp_file);
}

static void add_file_to_playlist(GmpvMpv *mpv, const gchar *uri)
{
	GmpvPlaylistEntry *entry = gmpv_playlist_entry_new(uri, NULL);

	g_ptr_array_add(get_private(mpv)->playlist, entry);
	g_signal_emit_by_name(mpv, "mpv-prop-change", "playlist", NULL);
}

static void update_playlist(GmpvMpv *mpv)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	const mpv_node_list *org_list;
	mpv_node playlist;

	g_ptr_array_set_size(priv->playlist, 0);
	gmpv_mpv_get_property(mpv, "playlist", MPV_FORMAT_NODE, &playlist);

	org_list = playlist.u.list;

	if(playlist.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			GmpvPlaylistEntry *entry;

			entry = parse_playlist_entry(org_list->values[i].u.list);
			g_ptr_array_add(priv->playlist, entry);
		}

		mpv_free_node_contents(&playlist);
	}
}

static void update_metadata(GmpvMpv *mpv)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	mpv_node_list *org_list = NULL;
	mpv_node metadata;

	g_ptr_array_set_size(priv->metadata, 0);
	gmpv_mpv_get_property(mpv, "metadata", MPV_FORMAT_NODE, &metadata);
	org_list = metadata.u.list;

	if(metadata.format == MPV_FORMAT_NODE_MAP && org_list->num > 0)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			const gchar *key;
			mpv_node value;

			key = org_list->keys[i];
			value = org_list->values[i];

			if(value.format == MPV_FORMAT_STRING)
			{
				GmpvMetadataEntry *entry;

				entry =	gmpv_metadata_entry_new
					(key, value.u.string);

				g_ptr_array_add(priv->metadata, entry);
			}
			else
			{
				g_warning(	"Ignored metadata field %s "
						"with unexpected format %d",
						key,
						value.format );
			}
		}

		mpv_free_node_contents(&metadata);
	}
}

static void update_track_list(GmpvMpv *mpv)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	mpv_node_list *org_list = NULL;
	mpv_node track_list;

	g_ptr_array_set_size(priv->track_list, 0);
	gmpv_mpv_get_property(mpv, "track-list", MPV_FORMAT_NODE, &track_list);
	org_list = track_list.u.list;

	if(track_list.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			GmpvTrack *entry =	parse_track_entry
						(org_list->values[i].u.list);

			g_ptr_array_add(priv->track_list, entry);
		}

		mpv_free_node_contents(&track_list);
	}
}

static void gmpv_mpv_class_init(GmpvMpvClass* klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

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
			"Whether mpv is initialized and ready to recieve commands",
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
	g_signal_new(	"mpv-video-reconfig",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"mpv-playback-restart",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"mpv-prop-change",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
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

static void gmpv_mpv_init(GmpvMpv *mpv)
{
	GmpvMpvPrivate *priv = get_private(mpv);

	priv->mpv_ctx = mpv_create();
	priv->opengl_ctx = NULL;
	priv->playlist = g_ptr_array_new_full(	1,
						(GDestroyNotify)
						gmpv_playlist_entry_free );
	priv->metadata = g_ptr_array_new_full(	1,
						(GDestroyNotify)
						gmpv_metadata_entry_free );
	priv->track_list = g_ptr_array_new_full(	1,
						(GDestroyNotify)
						gmpv_track_free );
	priv->tmp_input_file = NULL;
	priv->log_level_list = NULL;

	priv->ready = FALSE;
	priv->loaded = FALSE;
	priv->new_file = TRUE;

	priv->init_vo_config = TRUE;
	priv->force_opengl = FALSE;
	priv->use_opengl = FALSE;
	priv->wid = -1;
	priv->opengl_cb_callback_data = NULL;
	priv->opengl_cb_callback = NULL;
}

GmpvMpv *gmpv_mpv_new(gint64 wid)
{
	return GMPV_MPV(g_object_new(gmpv_mpv_get_type(), "wid", wid, NULL));
}

inline mpv_opengl_cb_context *gmpv_mpv_get_opengl_cb_context(GmpvMpv *mpv)
{
	return get_private(mpv)->opengl_ctx;
}

inline gboolean gmpv_mpv_get_use_opengl_cb(GmpvMpv *mpv)
{
	return get_private(mpv)->use_opengl;
}

GPtrArray *gmpv_mpv_get_metadata(GmpvMpv *mpv)
{
	return get_private(mpv)->metadata;
}

GPtrArray *gmpv_mpv_get_playlist(GmpvMpv *mpv)
{
	return get_private(mpv)->playlist;
}

GPtrArray *gmpv_mpv_get_track_list(GmpvMpv *mpv)
{
	return get_private(mpv)->track_list;
}

void gmpv_mpv_initialize(GmpvMpv *mpv)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	GSettings *settings = g_settings_new(CONFIG_ROOT);
	gchar *current_vo = NULL;
	gchar *mpv_version = NULL;

	apply_default_options(mpv);
	load_input_config_file(mpv);
	load_mpv_config_file(mpv);
	apply_extra_options(mpv);

	if(priv->force_opengl || priv->wid <= 0)
	{
		g_info("Forcing --vo=opengl-cb");
		mpv_set_option_string(priv->mpv_ctx, "vo", "opengl-cb");
	}
	else
	{
		g_debug("Attaching mpv window to wid %#x", (guint)priv->wid);
		mpv_set_option(priv->mpv_ctx, "wid", MPV_FORMAT_INT64, &priv->wid);
	}

	observe_properties(mpv);
	mpv_set_wakeup_callback(priv->mpv_ctx, wakeup_callback, mpv);
	mpv_initialize(priv->mpv_ctx);

	mpv_version = gmpv_mpv_get_property_string(mpv, "mpv-version");
	current_vo = gmpv_mpv_get_property_string(mpv, "current-vo");
	priv->use_opengl = !current_vo;

	g_info("Using %s", mpv_version);

	if(!priv->use_opengl && !GDK_IS_X11_DISPLAY(gdk_display_get_default()))
	{
		g_info(	"The chosen vo is %s but the display is not X11; "
			"forcing --vo=opengl-cb and resetting",
			current_vo );

		priv->force_opengl = TRUE;

		gmpv_mpv_reset(mpv);
	}
	else
	{
		GSettings *win_settings;
		gdouble volume;

		win_settings = g_settings_new(CONFIG_WIN_STATE);
		volume = g_settings_get_double(win_settings, "volume")*100;

		g_debug("Setting volume to %f", volume);
		mpv_set_property(	priv->mpv_ctx,
					"volume",
					MPV_FORMAT_DOUBLE,
					&volume );

		if(priv->use_opengl)
		{
			priv->opengl_ctx =	mpv_get_sub_api
						(	priv->mpv_ctx,
							MPV_SUB_API_OPENGL_CB );
		}

		gmpv_mpv_options_init(mpv);

		priv->force_opengl = FALSE;
		priv->ready = TRUE;
		g_object_notify(G_OBJECT(mpv), "ready");

		g_object_unref(win_settings);
	}

	g_object_unref(settings);
	mpv_free(current_vo);
	mpv_free(mpv_version);
}

void gmpv_mpv_init_gl(GmpvMpv *mpv)
{
	mpv_opengl_cb_context *opengl_ctx = gmpv_mpv_get_opengl_cb_context(mpv);
	gint rc = mpv_opengl_cb_init_gl(	opengl_ctx,
						"GL_MP_MPGetNativeDisplay",
						get_proc_address,
						NULL );

	if(rc >= 0)
	{
		g_debug("Initialized opengl-cb");
	}
	else
	{
		g_critical("Failed to initialize opengl-cb");
	}
}

void gmpv_mpv_reset(GmpvMpv *mpv)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	const gchar *quit_cmd[] = {"quit_watch_later", NULL};
	gchar *loop_str;
	gboolean loop;
	gboolean pause;
	gint64 playlist_pos;
	gint playlist_pos_rc;

	loop_str = gmpv_mpv_get_property_string(mpv, "loop");
	loop = (g_strcmp0(loop_str, "inf") == 0);
	pause = gmpv_mpv_get_property_flag(mpv, "pause");

	mpv_free(loop_str);

	playlist_pos_rc = mpv_get_property(	priv->mpv_ctx,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );

	/* Reset priv->mpv_ctx */
	priv->ready = FALSE;
	g_object_notify(G_OBJECT(mpv), "ready");

	gmpv_mpv_command(mpv, quit_cmd);
	gmpv_mpv_quit(mpv);

	priv->mpv_ctx = mpv_create();
	gmpv_mpv_initialize(mpv);

	gmpv_mpv_set_opengl_cb_callback
		(	mpv,
			priv->opengl_cb_callback,
			priv->opengl_cb_callback_data );

	gmpv_mpv_set_property_string(mpv, "loop", loop?"inf":"no");

	if(priv->playlist)
	{
		if(priv->loaded)
		{
			mpv_request_event(priv->mpv_ctx, MPV_EVENT_FILE_LOADED, 0);
			load_from_playlist(mpv);
			mpv_request_event(priv->mpv_ctx, MPV_EVENT_FILE_LOADED, 1);
		}

		if(playlist_pos_rc >= 0 && playlist_pos > 0)
		{
			gmpv_mpv_set_property(	mpv,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );
		}

		gmpv_mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &pause);
	}
}

void gmpv_mpv_quit(GmpvMpv *mpv)
{
	GmpvMpvPrivate *priv = get_private(mpv);

	g_info("Terminating mpv");

	if(priv->tmp_input_file)
	{
		g_unlink(priv->tmp_input_file);
	}

	if(priv->opengl_ctx)
	{
		g_debug("Uninitializing opengl-cb");
		mpv_opengl_cb_uninit_gl(priv->opengl_ctx);

		priv->opengl_ctx = NULL;
	}

	g_assert(priv->mpv_ctx);
	mpv_terminate_destroy(priv->mpv_ctx);

	priv->mpv_ctx = NULL;
}

void gmpv_mpv_load_track(GmpvMpv *mpv, const gchar *uri, TrackType type)
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
	gmpv_mpv_command(mpv, cmd);

	g_free(path);
}

void gmpv_mpv_load_file(GmpvMpv *mpv, const gchar *uri, gboolean append)
{
	GmpvMpvPrivate *priv = get_private(mpv);
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
		priv->new_file = TRUE;
		priv->loaded = FALSE;

		gmpv_mpv_set_property_flag(mpv, "pause", FALSE);
	}

	g_assert(priv->mpv_ctx);
	mpv_request_event(priv->mpv_ctx, MPV_EVENT_END_FILE, 0);
	mpv_command(priv->mpv_ctx, load_cmd);
	mpv_request_event(priv->mpv_ctx, MPV_EVENT_END_FILE, 1);

	g_free(path);
}

void gmpv_mpv_load(GmpvMpv *mpv, const gchar *uri, gboolean append)
{
	GmpvMpvPrivate *priv = get_private(mpv);
	const gchar *subtitle_exts[] = SUBTITLE_EXTS;

	if(extension_matches(uri, subtitle_exts))
	{
		gmpv_mpv_load_track(mpv, uri, TRACK_TYPE_SUBTITLE);
	}
	else
	{
		gboolean idle_active = FALSE;

		mpv_get_property(	priv->mpv_ctx,
					"idle-active",
					MPV_FORMAT_FLAG,
					&idle_active );

		if(idle_active || !priv->ready)
		{
			if(!append)
			{
				g_ptr_array_set_size(priv->playlist, 0);
			}

			add_file_to_playlist(mpv, uri);
		}
		else
		{
			gmpv_mpv_load_file(mpv, uri, append);
		}
	}
}
