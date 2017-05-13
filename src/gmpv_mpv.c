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
#include "gmpv_mpv_opt.h"
#include "gmpv_common.h"
#include "gmpv_def.h"
#include "gmpv_marshal.h"

static void *GLAPIENTRY glMPGetNativeDisplay(const gchar *name);
static void *get_proc_address(void *fn_ctx, const gchar *name);
static void set_inst_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec );
static void get_inst_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec );
static void load_from_playlist(GmpvMpv *mpv);
static void wakeup_callback(void *data);
static GmpvPlaylistEntry *parse_playlist_entry(mpv_node_list *node);
static GmpvTrack *parse_track_entry(mpv_node_list *node);
static void mpv_prop_change_handler(GmpvMpv *mpv, mpv_event_property* prop);
static gboolean mpv_event_handler(gpointer data);
static gint apply_args(mpv_handle *mpv_ctx, gchar *args);
static void log_handler(GmpvMpv *mpv, mpv_event_log_message* message);
static void load_input_conf(GmpvMpv *mpv, const gchar *input_conf);
static GPtrArray *get_mpv_playlist(GmpvMpv *mpv);

G_DEFINE_TYPE(GmpvMpv, gmpv_mpv, G_TYPE_OBJECT)

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

static void set_inst_property(	GObject *object,
				guint property_id,
				const GValue *value,
				GParamSpec *pspec )
{
	GmpvMpv *self = GMPV_MPV_OBJ(object);

	if(property_id == PROP_WID)
	{
		self->wid = g_value_get_int64(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void get_inst_property(	GObject *object,
				guint property_id,
				GValue *value,
				GParamSpec *pspec )
{
	GmpvMpv *self = GMPV_MPV_OBJ(object);

	if(property_id == PROP_WID)
	{
		g_value_set_int64(value, self->wid);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void load_from_playlist(GmpvMpv *mpv)
{
	GPtrArray * playlist = mpv->playlist;

	if(!mpv->state.init_load)
	{
		gmpv_mpv_set_property_flag(mpv, "pause", FALSE);
	}

	for(guint i = 0; playlist && i < playlist->len; i++)
	{
		GmpvPlaylistEntry *entry;

		entry = g_ptr_array_index(playlist, i);

		/* Do not append on first iteration */
		gmpv_mpv_load_file(mpv, entry->filename, i != 0, FALSE);
	}
}

static void wakeup_callback(void *data)
{
	g_idle_add((GSourceFunc)mpv_event_handler, data);
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
	g_debug("Received mpv property change event for \"%s\"", prop->name);

	if(g_strcmp0(prop->name, "pause") == 0)
	{
		gboolean idle_active;

		mpv->state.paused = prop->data?*((int *)prop->data):TRUE;

		mpv_get_property(	mpv->mpv_ctx,
					"idle-active",
					MPV_FORMAT_FLAG,
					&idle_active );

		if(idle_active && !mpv->state.paused)
		{
			load_from_playlist(mpv);
		}
	}
	else if(g_strcmp0(prop->name, "playlist") == 0)
	{
		gint64 playlist_count  = 0;
		gboolean idle_active = FALSE;
		gboolean was_empty = FALSE;

		mpv_get_property(	mpv->mpv_ctx,
					"playlist-count",
					MPV_FORMAT_INT64,
					&playlist_count );
		mpv_get_property(	mpv->mpv_ctx,
					"idle-active",
					MPV_FORMAT_FLAG,
					&idle_active );

		was_empty = (mpv->playlist->len == 0);

		if(!idle_active)
		{
			if(mpv->playlist)
			{
				g_ptr_array_free(mpv->playlist, TRUE);
			}

			mpv->playlist = get_mpv_playlist(mpv);
		}

		/* Check if we're transitioning from empty playlist to non-empty
		 * playlist.
		 */
		if(was_empty && mpv->playlist->len > 0)
		{
			gmpv_mpv_set_property_flag(mpv, "pause", FALSE);
		}
	}
	else if(g_strcmp0(prop->name, "vo-configured") == 0)
	{
		if(mpv->init_vo_config)
		{
			mpv->init_vo_config = FALSE;
			load_from_playlist(mpv);
		}
	}
}

static gboolean mpv_event_handler(gpointer data)
{
	GmpvMpv *mpv = data;
	gboolean done = !mpv;

	while(!done)
	{
		mpv_event *event =	mpv->mpv_ctx?
					mpv_wait_event(mpv->mpv_ctx, 0):
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
			if(mpv->state.loaded)
			{
				mpv->state.loaded = FALSE;

				gmpv_mpv_set_property_flag
					(mpv, "pause", TRUE);
			}

			if(!mpv->state.init_load && !mpv->state.loaded)
			{
				g_signal_emit_by_name(mpv, "idle");
			}

			mpv->state.init_load = FALSE;
		}
		else if(event->event_id == MPV_EVENT_FILE_LOADED)
		{
			mpv->state.loaded = TRUE;
			mpv->state.init_load = FALSE;

			g_signal_emit_by_name(mpv, "load");
		}
		else if(event->event_id == MPV_EVENT_END_FILE)
		{
			mpv_event_end_file *ef_event = event->data;

			mpv->state.init_load = FALSE;

			if(mpv->state.loaded)
			{
				mpv->state.new_file = FALSE;
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

				gmpv_mpv_set_property_flag
					(mpv, "pause", TRUE);
				g_signal_emit_by_name(mpv, "mpv-error", msg);

				g_free(msg);
			}
		}
		else if(event->event_id == MPV_EVENT_VIDEO_RECONFIG)
		{
			if(mpv->state.new_file)
			{
				gmpv_mpv_opt_handle_autofit(mpv);

				g_signal_emit_by_name(	mpv,
							"autofit",
							mpv->autofit_ratio );
			}
		}
		else if(event->event_id == MPV_EVENT_START_FILE)
		{
			gboolean vo_configured = FALSE;

			mpv_get_property(	mpv->mpv_ctx,
						"vo-configured",
						MPV_FORMAT_FLAG,
						&vo_configured );

			/* If the vo is not configured yet, save the content of
			 * mpv's playlist in mpv->playlist. This will be loaded
			 * again when the vo is configured.
			 */
			if(!vo_configured)
			{
				if(mpv->playlist)
				{
					g_ptr_array_free(mpv->playlist, TRUE);
				}

				mpv->playlist = get_mpv_playlist(mpv);
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

		if(event && !mpv->mpv_ctx)
		{
			done = TRUE;
		}
	}

	return FALSE;
}

static gint apply_args(mpv_handle *mpv_ctx, gchar *args)
{
	gchar *opt_begin = args?strstr(args, "--"):NULL;
	gint fail_count = 0;

	while(opt_begin)
	{
		gchar *opt_end = strstr(opt_begin, " --");
		gchar *token;
		gchar *token_arg;
		gsize token_size;

		/* Point opt_end to the end of the input string if the current
		 * option is the last one.
		 */
		if(!opt_end)
		{
			opt_end = args+strlen(args);
		}

		/* Traverse the string backwards until non-space character is
		 * found. This removes spaces after the option token.
		 */
		while(	--opt_end != opt_begin
			&& (*opt_end == ' ' || *opt_end == '\n') );

		token_size = (gsize)(opt_end-opt_begin);
		token = g_strndup(opt_begin+2, token_size-1);
		token_arg = strpbrk(token, "= ");

		if(token_arg)
		{
			*token_arg = '\0';

			token_arg++;
		}
		else
		{
			/* Default to empty string if there is no explicit
			 * argument
			 */
			token_arg = "";
		}

		g_debug("Applying option --%s=%s", token, token_arg);

		if(mpv_set_option_string(mpv_ctx, token, token_arg) < 0)
		{
			fail_count++;

			g_warning(	"Failed to apply option: --%s=%s\n",
					token,
					token_arg );
		}

		opt_begin = strstr(opt_end, " --");

		if(opt_begin)
		{
			opt_begin++;
		}

		g_free(token);
	}

	return fail_count*(-1);
}

static void log_handler(GmpvMpv *mpv, mpv_event_log_message* message)
{
	GSList *iter = mpv->log_level_list;
	module_log_level *level = NULL;
	gsize event_prefix_len = strlen(message->prefix);
	gboolean found = FALSE;

	if(iter)
	{
		level = iter->data;
	}

	while(iter && !found)
	{
		gsize prefix_len = strlen(level->prefix);
		gint cmp;

		cmp = strncmp(	level->prefix,
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
	const gchar *default_keybinds[] = DEFAULT_KEYBINDS;
	gchar *tmp_path;
	FILE *tmp_file;
	gint tmp_fd;

	tmp_fd = g_file_open_tmp(NULL, &tmp_path, NULL);
	tmp_file = fdopen(tmp_fd, "w");
	mpv->tmp_input_file = tmp_path;

	if(!tmp_file)
	{
		g_error("Failed to open temporary input config file");
	}

	for(gint i = 0; default_keybinds[i]; i++)
	{
		const gsize len = strlen(default_keybinds[i]);
		gsize write_size;
		gint rc;

		write_size = fwrite(default_keybinds[i], len, 1, tmp_file);
		rc = fputc('\n', tmp_file);

		if(write_size != 1 || rc != '\n')
		{
			g_error(	"Error writing default keybindings to "
					"temporary input config file" );
		}
	}

	g_debug("Using temporary input config file: %s", tmp_path);
	mpv_set_option_string(mpv->mpv_ctx, "input-conf", tmp_path);

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

static GPtrArray *get_mpv_playlist(GmpvMpv *mpv)
{
	GPtrArray *result = NULL;
	const mpv_node_list *org_list;
	mpv_node playlist;

	result = g_ptr_array_new_full(	1,
					(GDestroyNotify)
					gmpv_playlist_entry_free );

	gmpv_mpv_get_property(mpv, "playlist", MPV_FORMAT_NODE, &playlist);
	org_list = playlist.u.list;

	if(playlist.format == MPV_FORMAT_NODE_ARRAY)
	{
		for(gint i = 0; i < org_list->num; i++)
		{
			GmpvPlaylistEntry *entry;

			entry = parse_playlist_entry(org_list->values[i].u.list);
			g_ptr_array_add(result, entry);
		}

		mpv_free_node_contents(&playlist);
	}

	return result;
}

static void gmpv_mpv_class_init(GmpvMpvClass* klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->set_property = set_inst_property;
	obj_class->get_property = get_inst_property;

	pspec = g_param_spec_int64
		(	"wid",
			"WID",
			"The ID of the window to attach to",
			G_MININT64,
			G_MAXINT64,
			-1,
			G_PARAM_READWRITE );
	g_object_class_install_property(obj_class, PROP_WID, pspec);

	g_signal_new(	"mpv-init",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"mpv-error",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
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

	g_signal_new(	"idle",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
	g_signal_new(	"load",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0 );
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
	g_signal_new(	"autofit",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__DOUBLE,
			G_TYPE_NONE,
			1,
			G_TYPE_DOUBLE );
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
	mpv->mpv_ctx = mpv_create();
	mpv->opengl_ctx = NULL;
	mpv->playlist = g_ptr_array_new_full(	1,
						(GDestroyNotify)
						gmpv_metadata_entry_free );
	mpv->tmp_input_file = NULL;
	mpv->log_level_list = NULL;
	mpv->autofit_ratio = -1;
	mpv->geometry = NULL;

	mpv->state.ready = FALSE;
	mpv->state.paused = TRUE;
	mpv->state.loaded = FALSE;
	mpv->state.new_file = TRUE;
	mpv->state.init_load = TRUE;

	mpv->init_vo_config = TRUE;
	mpv->force_opengl = FALSE;
	mpv->use_opengl = FALSE;
	mpv->glarea = NULL;
	mpv->wid = -1;
	mpv->opengl_cb_callback_data = NULL;
	mpv->opengl_cb_callback = NULL;
}

GmpvMetadataEntry *gmpv_metadata_entry_new(const gchar *key, const gchar *value)
{
	GmpvMetadataEntry *entry = g_malloc(sizeof(GmpvMetadataEntry));

	entry->key = g_strdup(key);
	entry->value = g_strdup(value);

	return entry;
}

void gmpv_metadata_entry_free(GmpvMetadataEntry *entry)
{
	g_free(entry->key);
	g_free(entry->value);
	g_free(entry);
}

GmpvMpv *gmpv_mpv_new(gint64 wid)
{
	return GMPV_MPV_OBJ(g_object_new(gmpv_mpv_get_type(), "wid", wid, NULL));
}

inline const GmpvMpvState *gmpv_mpv_get_state(GmpvMpv *mpv)
{
	return &mpv->state;
}

inline GmpvGeometry *gmpv_mpv_get_geometry(GmpvMpv *mpv)
{
	return mpv->geometry;
}

inline mpv_handle *gmpv_mpv_get_mpv_handle(GmpvMpv *mpv)
{
	return mpv->mpv_ctx;
}

inline mpv_opengl_cb_context *gmpv_mpv_get_opengl_cb_context(GmpvMpv *mpv)
{
	return mpv->opengl_ctx;
}

inline gboolean gmpv_mpv_get_use_opengl_cb(GmpvMpv *mpv)
{
	return mpv->use_opengl;
}

GPtrArray *gmpv_mpv_get_metadata(GmpvMpv *mpv)
{
	GPtrArray *result = NULL;
	mpv_node_list *org_list = NULL;
	mpv_node metadata;

	gmpv_mpv_get_property(mpv, "metadata", MPV_FORMAT_NODE, &metadata);
	org_list = metadata.u.list;

	if(metadata.format == MPV_FORMAT_NODE_MAP && org_list->num > 0)
	{
		result = g_ptr_array_new_full(	(guint)
						org_list->num,
						(GDestroyNotify)
						gmpv_metadata_entry_free );

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

				g_ptr_array_add(result, entry);
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

	return result;
}

GPtrArray *gmpv_mpv_get_playlist(GmpvMpv *mpv)
{
	return mpv->playlist;
}

GPtrArray *gmpv_mpv_get_track_list(GmpvMpv *mpv)
{
	GPtrArray *result = NULL;
	mpv_node_list *org_list = NULL;
	mpv_node track_list;

	gmpv_mpv_get_property(mpv, "track-list", MPV_FORMAT_NODE, &track_list);
	org_list = track_list.u.list;

	if(track_list.format == MPV_FORMAT_NODE_ARRAY)
	{
		result = g_ptr_array_new_full(	(guint)
						org_list->num,
						(GDestroyNotify)
						gmpv_track_free );

		for(gint i = 0; i < org_list->num; i++)
		{
			GmpvTrack *entry =	parse_track_entry
						(org_list->values[i].u.list);

			g_ptr_array_add(result, entry);
		}

		mpv_free_node_contents(&track_list);
	}

	return result;
}

void gmpv_mpv_initialize(GmpvMpv *mpv)
{
	GSettings *main_settings = g_settings_new(CONFIG_ROOT);
	gchar *config_dir = get_config_dir_path();
	gchar *mpvopt = NULL;
	gchar *current_vo = NULL;
	gchar *mpv_version = NULL;

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
			{"osd-bar", "no"},
			{"input-cursor", "no"},
			{"cursor-autohide", "no"},
			{"softvol-max", "100"},
			{"config", "yes"},
			{"screenshot-template", "gnome-mpv-shot%n"},
			{"config-dir", config_dir},
			{NULL, NULL} };

	g_assert(mpv->mpv_ctx);

	for(gint i = 0; options[i].name; i++)
	{
		g_debug(	"Applying default option --%s=%s",
				options[i].name,
				options[i].value );

		mpv_set_option_string(	mpv->mpv_ctx,
					options[i].name,
					options[i].value );
	}

	if(g_settings_get_boolean(main_settings, "mpv-config-enable"))
	{
		gchar *mpv_conf
			= g_settings_get_string
				(main_settings, "mpv-config-file");

		g_info("Loading config file: %s", mpv_conf);
		mpv_load_config_file(mpv->mpv_ctx, mpv_conf);

		g_free(mpv_conf);
	}

	if(g_settings_get_boolean(main_settings, "mpv-input-config-enable"))
	{
		gchar *input_conf
			= g_settings_get_string
				(main_settings, "mpv-input-config-file");

		g_info("Loading input config file: %s", input_conf);
		load_input_conf(mpv, input_conf);

		g_free(input_conf);
	}
	else
	{
		load_input_conf(mpv, NULL);
	}

	mpvopt = g_settings_get_string(main_settings, "mpv-options");

	g_debug("Applying extra mpv options: %s", mpvopt);

	/* Apply extra options */
	if(apply_args(mpv->mpv_ctx, mpvopt) < 0)
	{
		const gchar *msg
			= _("Failed to apply one or more MPV options.");

		g_signal_emit_by_name(mpv, "mpv-error", msg);
	}

	if(mpv->force_opengl || mpv->wid <= 0)
	{
		g_info("Forcing --vo=opengl-cb");
		mpv_set_option_string(mpv->mpv_ctx, "vo", "opengl-cb");

	}
	else
	{
		g_debug(	"Attaching mpv window to wid %#x",
				(guint)mpv->wid );

		mpv_set_option(mpv->mpv_ctx, "wid", MPV_FORMAT_INT64, &mpv->wid);
	}

	mpv_observe_property(mpv->mpv_ctx, 0, "aid", MPV_FORMAT_STRING);
	mpv_observe_property(mpv->mpv_ctx, 0, "vid", MPV_FORMAT_STRING);
	mpv_observe_property(mpv->mpv_ctx, 0, "sid", MPV_FORMAT_STRING);
	mpv_observe_property(mpv->mpv_ctx, 0, "chapters", MPV_FORMAT_INT64);
	mpv_observe_property(mpv->mpv_ctx, 0, "core-idle", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv->mpv_ctx, 0, "idle-active", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv->mpv_ctx, 0, "fullscreen", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv->mpv_ctx, 0, "pause", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv->mpv_ctx, 0, "loop", MPV_FORMAT_STRING);
	mpv_observe_property(mpv->mpv_ctx, 0, "duration", MPV_FORMAT_DOUBLE);
	mpv_observe_property(mpv->mpv_ctx, 0, "media-title", MPV_FORMAT_STRING);
	mpv_observe_property(mpv->mpv_ctx, 0, "metadata", MPV_FORMAT_NODE);
	mpv_observe_property(mpv->mpv_ctx, 0, "playlist", MPV_FORMAT_NODE);
	mpv_observe_property(mpv->mpv_ctx, 0, "playlist-count", MPV_FORMAT_INT64);
	mpv_observe_property(mpv->mpv_ctx, 0, "playlist-pos", MPV_FORMAT_INT64);
	mpv_observe_property(mpv->mpv_ctx, 0, "speed", MPV_FORMAT_DOUBLE);
	mpv_observe_property(mpv->mpv_ctx, 0, "track-list", MPV_FORMAT_NODE);
	mpv_observe_property(mpv->mpv_ctx, 0, "vo-configured", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv->mpv_ctx, 0, "volume", MPV_FORMAT_DOUBLE);
	mpv_set_wakeup_callback(mpv->mpv_ctx, wakeup_callback, mpv);
	mpv_initialize(mpv->mpv_ctx);

	mpv_version = gmpv_mpv_get_property_string(mpv, "mpv-version");
	current_vo = gmpv_mpv_get_property_string(mpv, "current-vo");
	mpv->use_opengl = !current_vo;

	g_info("Using %s", mpv_version);

	if(!mpv->use_opengl && !GDK_IS_X11_DISPLAY(gdk_display_get_default()))
	{
		g_info(	"The chosen vo is %s but the display is not X11; "
			"forcing --vo=opengl-cb and resetting",
			current_vo );

		mpv->force_opengl = TRUE;
		mpv->state.paused = FALSE;

		gmpv_mpv_reset(mpv);
	}
	else
	{
		GSettings *win_settings;
		gdouble volume;

		win_settings = g_settings_new(CONFIG_WIN_STATE);
		volume = g_settings_get_double(win_settings, "volume")*100;

		g_debug("Setting volume to %f", volume);
		mpv_set_property(	mpv->mpv_ctx,
					"volume",
					MPV_FORMAT_DOUBLE,
					&volume );

		if(mpv->use_opengl)
		{
			mpv->opengl_ctx =	mpv_get_sub_api
						(	mpv->mpv_ctx,
							MPV_SUB_API_OPENGL_CB );
		}

		gmpv_mpv_opt_handle_msg_level(mpv);
		gmpv_mpv_opt_handle_fs(mpv);
		gmpv_mpv_opt_handle_geometry(mpv);

		mpv->force_opengl = FALSE;
		mpv->state.ready = TRUE;
		g_signal_emit_by_name(mpv, "mpv-init");

		g_clear_object(&win_settings);
	}

	g_clear_object(&main_settings);
	g_free(config_dir);
	g_free(mpvopt);
	mpv_free(current_vo);
	mpv_free(mpv_version);
}

void gmpv_mpv_init_gl(GmpvMpv *mpv)
{
	mpv_opengl_cb_context *opengl_ctx;
	gint rc;

	opengl_ctx = gmpv_mpv_get_opengl_cb_context(mpv);
	rc = mpv_opengl_cb_init_gl(	opengl_ctx,
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
	const gchar *quit_cmd[] = {"quit_watch_later", NULL};
	gchar *loop_str;
	gboolean loop;
	gint64 playlist_pos;
	gint playlist_pos_rc;

	g_assert(mpv->mpv_ctx);

	loop_str = gmpv_mpv_get_property_string(mpv, "loop");
	loop = (g_strcmp0(loop_str, "inf") == 0);

	mpv_free(loop_str);

	playlist_pos_rc = mpv_get_property(	mpv->mpv_ctx,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );

	/* Reset mpv->mpv_ctx */
	mpv->state.ready = FALSE;

	gmpv_mpv_command(mpv, quit_cmd);
	gmpv_mpv_quit(mpv);

	mpv->mpv_ctx = mpv_create();
	gmpv_mpv_initialize(mpv);

	gmpv_mpv_set_opengl_cb_callback
		(	mpv,
			mpv->opengl_cb_callback,
			mpv->opengl_cb_callback_data );

	gmpv_mpv_set_property_string(mpv, "loop", loop?"inf":"no");

	if(mpv->playlist)
	{
		if(mpv->state.loaded)
		{
			mpv_request_event(mpv->mpv_ctx, MPV_EVENT_FILE_LOADED, 0);
			load_from_playlist(mpv);
			mpv_request_event(mpv->mpv_ctx, MPV_EVENT_FILE_LOADED, 1);
		}

		if(playlist_pos_rc >= 0 && playlist_pos > 0)
		{
			gmpv_mpv_set_property(	mpv,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );
		}

		gmpv_mpv_set_property(	mpv,
					"pause",
					MPV_FORMAT_FLAG,
					&mpv->state.paused );
	}
}

void gmpv_mpv_quit(GmpvMpv *mpv)
{
	g_info("Terminating mpv");

	if(mpv->tmp_input_file)
	{
		g_unlink(mpv->tmp_input_file);
	}

	if(mpv->opengl_ctx)
	{
		g_debug("Uninitializing opengl-cb");
		mpv_opengl_cb_uninit_gl(mpv->opengl_ctx);

		mpv->opengl_ctx = NULL;
	}

	g_assert(mpv->mpv_ctx);
	mpv_terminate_destroy(mpv->mpv_ctx);

	mpv->mpv_ctx = NULL;
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

void gmpv_mpv_load_file(	GmpvMpv *mpv,
				const gchar *uri,
				gboolean append,
				gboolean update )
{
	const gchar *load_cmd[] = {"loadfile", NULL, NULL, NULL};
	gint64 playlist_count = 0;

	g_info(	"Loading file (append=%s, update=%s): %s",
		append?"TRUE":"FALSE",
		update?"TRUE":"FALSE",
		uri?:"<PLAYLIST_ITEMS>" );

	mpv_get_property(	mpv->mpv_ctx,
				"playlist-count",
				MPV_FORMAT_INT64,
				&playlist_count );

	load_cmd[2] = (append && playlist_count > 0)?"append":"replace";

	if(!append && uri && update)
	{
		mpv->state.new_file = TRUE;
		mpv->state.loaded = FALSE;
	}

	if(!uri)
	{
		load_from_playlist(mpv);
	}

	if(uri)
	{
		gchar *path = get_path_from_uri(uri);

		load_cmd[1] = path;

		if(!append)
		{
			mpv->state.loaded = FALSE;

			if(!mpv->state.init_load)
			{
				gmpv_mpv_set_property_flag
					(mpv, "pause", FALSE);
			}
		}

		g_assert(mpv->mpv_ctx);

		mpv_request_event(mpv->mpv_ctx, MPV_EVENT_END_FILE, 0);
		mpv_command(mpv->mpv_ctx, load_cmd);
		mpv_request_event(mpv->mpv_ctx, MPV_EVENT_END_FILE, 1);

		g_free(path);
	}
}

void gmpv_mpv_load(	GmpvMpv *mpv,
			const gchar *uri,
			gboolean append,
			gboolean update )
{
	const gchar *subtitle_exts[] = SUBTITLE_EXTS;
	gboolean idle_active = FALSE;

	mpv_get_property(	mpv->mpv_ctx,
				"idle-active",
				MPV_FORMAT_FLAG,
				&idle_active );

	if(extension_matches(uri, subtitle_exts))
	{
		gmpv_mpv_load_track(mpv, uri, TRACK_TYPE_SUBTITLE);
	}
	else
	{
		if(append && idle_active && mpv->playlist->len > 0)
		{
			mpv_node playlist;
			GmpvPlaylistEntry *entry;

			entry = gmpv_playlist_entry_new(uri, NULL);
			g_ptr_array_add(mpv->playlist, entry);

			g_signal_emit_by_name(	mpv,
						"mpv-prop-change",
						"playlist",
						&playlist );

			mpv_free_node_contents(&playlist);
		}
		else
		{
			gmpv_mpv_load_file(mpv, uri, append, update);
		}
	}
}

void gmpv_mpv_free(gpointer data)
{
	mpv_free(data);
}
