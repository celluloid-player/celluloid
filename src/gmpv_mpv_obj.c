/*
 * Copyright (c) 2014-2016 gnome-mpv
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

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

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

#include "gmpv_mpv_obj.h"
#include "gmpv_mpv_obj_private.h"
#include "gmpv_mpv_opt.h"
#include "gmpv_application.h"
#include "gmpv_common.h"
#include "gmpv_def.h"
#include "gmpv_track.h"
#include "gmpv_playlist.h"
#include "gmpv_control_box.h"
#include "gmpv_playlist_widget.h"

static void gmpv_mpv_obj_set_inst_property(	GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec );
static void gmpv_mpv_obj_get_inst_property(	GObject *object,
					guint property_id,
					GValue *value,
					GParamSpec *pspec );
static void wakeup_callback(void *data);
static void mpv_prop_change_handler(GmpvMpvObj *mpv, mpv_event_property* prop);
static gboolean mpv_event_handler(gpointer data);
static void mpv_obj_update_playlist(GmpvMpvObj *mpv);
static gint mpv_obj_apply_args(mpv_handle *mpv_ctx, gchar *args);
static void mpv_obj_log_handler(GmpvMpvObj *mpv, mpv_event_log_message* message);
static void load_input_conf(GmpvMpvObj *mpv, const gchar *input_conf);

G_DEFINE_TYPE(GmpvMpvObj, gmpv_mpv_obj, G_TYPE_OBJECT)

static void gmpv_mpv_obj_set_inst_property(	GObject *object,
						guint property_id,
						const GValue *value,
						GParamSpec *pspec )
{
	GmpvMpvObj *self = GMPV_MPV_OBJ(object);

	if(property_id == PROP_WID)
	{
		self->wid = g_value_get_int64(value);
	}
	else if(property_id == PROP_PLAYLIST)
	{
		self->playlist = g_value_get_pointer(value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void gmpv_mpv_obj_get_inst_property(	GObject *object,
						guint property_id,
						GValue *value,
						GParamSpec *pspec )
{
	GmpvMpvObj *self = GMPV_MPV_OBJ(object);

	if(property_id == PROP_WID)
	{
		g_value_set_int64(value, self->wid);
	}
	else if(property_id == PROP_PLAYLIST)
	{
		g_value_set_pointer(value, self->playlist);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void wakeup_callback(void *data)
{
	g_idle_add((GSourceFunc)mpv_event_handler, data);
}

static void mpv_prop_change_handler(GmpvMpvObj *mpv, mpv_event_property* prop)
{
	if(g_strcmp0(prop->name, "pause") == 0)
	{
		gboolean idle;

		mpv->state.paused = prop->data?*((int *)prop->data):TRUE;

		mpv_get_property(mpv->mpv_ctx, "idle", MPV_FORMAT_FLAG, &idle);

		if(idle && !mpv->state.paused)
		{
			gmpv_mpv_obj_load(mpv, NULL, FALSE, TRUE);
		}
	}
}

static gboolean mpv_event_handler(gpointer data)
{
	GmpvMpvObj *mpv = data;
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
						g_strdup(prop->name) );
		}
		else if(event->event_id == MPV_EVENT_IDLE)
		{
			if(mpv->state.loaded)
			{
				mpv->state.loaded = FALSE;

				gmpv_mpv_obj_set_property_flag
					(mpv, "pause", TRUE);
				gmpv_playlist_reset(mpv->playlist);
			}

			mpv->state.init_load = FALSE;
		}
		else if(event->event_id == MPV_EVENT_FILE_LOADED)
		{
			mpv->state.loaded = TRUE;
			mpv->state.init_load = FALSE;

			mpv_obj_update_playlist(mpv);
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

				gmpv_mpv_obj_set_property_flag
					(mpv, "pause", TRUE);
				g_signal_emit_by_name(mpv, "mpv-error", msg);
			}
		}
		else if(event->event_id == MPV_EVENT_VIDEO_RECONFIG)
		{
			if(mpv->state.new_file)
			{
				gmpv_mpv_opt_handle_autofit(mpv);
			}
		}
		else if(event->event_id == MPV_EVENT_PLAYBACK_RESTART)
		{
			g_signal_emit_by_name(mpv, "mpv-playback-restart");
		}
		else if(event->event_id == MPV_EVENT_LOG_MESSAGE)
		{
			mpv_obj_log_handler(mpv, event->data);
		}
		else if(event->event_id == MPV_EVENT_SHUTDOWN
		|| event->event_id == MPV_EVENT_NONE)
		{
			done = TRUE;
		}

		if(event)
		{
			if(mpv->event_callback)
			{
				mpv->event_callback
					(event, mpv->event_callback_data);
			}

			if(mpv->mpv_ctx)
			{
				g_signal_emit_by_name
					(mpv, "mpv-event", event->event_id);
			}
			else
			{
				done = TRUE;
			}
		}
	}

	return FALSE;
}

static void mpv_obj_update_playlist(GmpvMpvObj *mpv)
{
	/* The length of "playlist//filename" including null-terminator (19)
	 * plus the number of digits in the maximum value of 64 bit int (19).
	 */
	const gsize filename_prop_str_size = 38;
	GtkListStore *store = gmpv_playlist_get_store(mpv->playlist);
	gchar *filename_prop_str = g_malloc(filename_prop_str_size);
	gboolean iter_end = FALSE;
	GtkTreeIter iter;
	mpv_node mpv_playlist;
	gint playlist_count;
	gint i;

	mpv_check_error(mpv_get_property(	mpv->mpv_ctx,
						"playlist",
						MPV_FORMAT_NODE,
						&mpv_playlist ));
	playlist_count = mpv_playlist.u.list->num;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	for(i = 0; i < playlist_count; i++)
	{
		gint prop_count = mpv_playlist.u.list->values[i].u.list->num;
		gchar *uri = NULL;
		gchar *title = NULL;
		gchar *name = NULL;
		gchar *old_name = NULL;
		gchar *old_uri = NULL;

		/* The first entry must always exist */
		uri =	mpv_playlist.u.list
			->values[i].u.list
			->values[0].u.string;

		/* Try retrieving the title from mpv playlist */
		if(prop_count >= 4)
		{
			title = mpv_playlist.u.list
				->values[i].u.list
				->values[3].u.string;
		}

		name = title?g_strdup(title):get_name_from_path(uri);

		if(!iter_end)
		{
			gtk_tree_model_get
				(	GTK_TREE_MODEL(store), &iter,
					PLAYLIST_NAME_COLUMN, &old_name,
					PLAYLIST_URI_COLUMN, &old_uri, -1 );

			if(g_strcmp0(name, old_name) != 0)
			{
				gtk_list_store_set
					(	store, &iter,
						PLAYLIST_NAME_COLUMN, name, -1 );
			}

			if(g_strcmp0(uri, old_uri) != 0)
			{
				gtk_list_store_set
					(	store, &iter,
						PLAYLIST_URI_COLUMN, uri, -1 );
			}

			iter_end = !gtk_tree_model_iter_next
					(GTK_TREE_MODEL(store), &iter);

			g_free(old_name);
			g_free(old_uri);
		}
		/* Append entries to the playlist if there are fewer entries in
		 * the playlist widget than mpv's playlist.
		 */
		else
		{
			gmpv_playlist_append(mpv->playlist, name, uri);
		}

		g_free(name);
	}

	/* If there are more entries in the playlist widget than mpv's playlist,
	 * remove the excess entries from the playlist widget.
	 */
	if(!iter_end)
	{
		while(gtk_list_store_remove(store, &iter));
	}

	g_free(filename_prop_str);
	mpv_free_node_contents(&mpv_playlist);
}

static gint mpv_obj_apply_args(mpv_handle *mpv_ctx, gchar *args)
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

static void mpv_obj_log_handler(GmpvMpvObj *mpv, mpv_event_log_message* message)
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

static void load_input_conf(GmpvMpvObj *mpv, const gchar *input_conf)
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

static void gmpv_mpv_obj_class_init(GmpvMpvObjClass* klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec = NULL;

	obj_class->set_property = gmpv_mpv_obj_set_inst_property;
	obj_class->get_property = gmpv_mpv_obj_get_inst_property;

	pspec = g_param_spec_int64
		(	"wid",
			"WID",
			"The ID of the window to attach to",
			G_MININT64,
			G_MAXINT64,
			-1,
			G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_WID, pspec);

	pspec = g_param_spec_pointer
		(	"playlist",
			"Playlist",
			"GmpvPlaylist to use for storage",
			G_PARAM_READWRITE );

	g_object_class_install_property(obj_class, PROP_PLAYLIST, pspec);

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

	g_signal_new(	"mpv-event",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__ENUM,
			G_TYPE_NONE,
			1,
			G_TYPE_INT );

	g_signal_new(	"mpv-prop-change",
			G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_FIRST,
			0,
			NULL,
			NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1,
			G_TYPE_STRING );
}

static void gmpv_mpv_obj_init(GmpvMpvObj *mpv)
{
	mpv->mpv_ctx = mpv_create();
	mpv->opengl_ctx = NULL;
	mpv->playlist = NULL;
	mpv->tmp_input_file = NULL;
	mpv->log_level_list = NULL;
	mpv->autofit_ratio = -1;

	mpv->state.ready = FALSE;
	mpv->state.paused = TRUE;
	mpv->state.loaded = FALSE;
	mpv->state.new_file = TRUE;
	mpv->state.init_load = TRUE;

	mpv->force_opengl = FALSE;
	mpv->glarea = NULL;
	mpv->wid = -1;
	mpv->event_callback_data = NULL;
	mpv->opengl_cb_callback_data = NULL;
	mpv->event_callback = NULL;
	mpv->opengl_cb_callback = NULL;
}

GmpvMpvObj *gmpv_mpv_obj_new(	GmpvPlaylist *playlist,
			gint64 wid )
{
	return GMPV_MPV_OBJ(g_object_new(	gmpv_mpv_obj_get_type(),
						"playlist", playlist,
					"wid", wid,
					NULL ));
}

gint gmpv_mpv_obj_command(GmpvMpvObj *mpv, const gchar **cmd)
{
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(mpv->mpv_ctx)
	{
		rc = mpv_command(mpv->mpv_ctx, cmd);
	}

	if(rc < 0)
	{
		gchar *cmd_str = g_strjoinv(" ", (gchar **)cmd);

		g_warning(	"Failed to run mpv command \"%s\". Reason: %s.",
				cmd_str,
				mpv_error_string(rc) );

		g_free(cmd_str);
	}

	return rc;
}

gint gmpv_mpv_obj_command_string(GmpvMpvObj *mpv, const gchar *cmd)
{
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(mpv->mpv_ctx)
	{
		rc = mpv_command_string(mpv->mpv_ctx, cmd);
	}

	if(rc < 0)
	{
		g_warning(	"Failed to run mpv command string \"%s\". "
				"Reason: %s.",
				cmd,
				mpv_error_string(rc) );
	}

	return rc;
}

gint gmpv_mpv_obj_get_property(	GmpvMpvObj *mpv,
				const gchar *name,
				mpv_format format,
				void *data )
{
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(mpv->mpv_ctx)
	{
		rc = mpv_get_property(mpv->mpv_ctx, name, format, data);
	}

	if(rc < 0)
	{
		g_info(	"Failed to retrieve property \"%s\" "
			"using mpv format %d. Reason: %s.",
			name,
			format,
			mpv_error_string(rc) );
	}

	return rc;
}

gchar *gmpv_mpv_obj_get_property_string(GmpvMpvObj *mpv, const gchar *name)
{
	gchar *value = NULL;

	if(mpv->mpv_ctx)
	{
		value = mpv_get_property_string(mpv->mpv_ctx, name);
	}

	if(!value)
	{
		g_info("Failed to retrieve property \"%s\" as string.", name);
	}

	return value;
}

gboolean gmpv_mpv_obj_get_property_flag(GmpvMpvObj *mpv, const gchar *name)
{
	gboolean value = FALSE;
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(mpv->mpv_ctx)
	{
		rc =	mpv_get_property
			(mpv->mpv_ctx, name, MPV_FORMAT_FLAG, &value);
	}

	if(rc < 0)
	{
		g_info(	"Failed to retrieve property \"%s\" as flag. "
			"Reason: %s.",
			name,
			mpv_error_string(rc) );
	}

	return value;
}

gint gmpv_mpv_obj_set_property(	GmpvMpvObj *mpv,
				const gchar *name,
				mpv_format format,
				void *data )
{
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(mpv->mpv_ctx)
	{
		rc = mpv_set_property(mpv->mpv_ctx, name, format, data);
	}

	if(rc < 0)
	{
		g_info(	"Failed to set property \"%s\" using mpv format %d. "
			"Reason: %s.",
			name,
			format,
			mpv_error_string(rc) );
	}

	return rc;
}

gint gmpv_mpv_obj_set_property_string(	GmpvMpvObj *mpv,
					const gchar *name,
					const char *data )
{
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(mpv->mpv_ctx)
	{
		rc = mpv_set_property_string(mpv->mpv_ctx, name, data);
	}

	if(rc < 0)
	{
		g_info(	"Failed to set property \"%s\" as string. Reason: %s.",
			name,
			mpv_error_string(rc) );
	}

	return rc;
}

gint gmpv_mpv_obj_set_property_flag(	GmpvMpvObj *mpv,
				const gchar *name,
				gboolean value )
{
	gint rc = MPV_ERROR_UNINITIALIZED;

	if(mpv->mpv_ctx)
	{
		rc =	mpv_set_property
			(mpv->mpv_ctx, name, MPV_FORMAT_FLAG, &value);
	}

	if(rc < 0)
	{
		g_info(	"Failed to set property \"%s\" as flag. Reason: %s.",
			name,
			mpv_error_string(rc) );
	}

	return rc;
}

void gmpv_mpv_obj_set_event_callback(	GmpvMpvObj *mpv,
					void (*func)(mpv_event *, void *),
					void *data )
{
	mpv->event_callback = func;
	mpv->event_callback_data = data;
}

void gmpv_mpv_obj_set_opengl_cb_callback(	GmpvMpvObj *mpv,
					mpv_opengl_cb_update_fn func,
					void *data )
{
	mpv->opengl_cb_callback = func;
	mpv->opengl_cb_callback_data = data;

	if(mpv->opengl_ctx)
	{
		mpv_opengl_cb_set_update_callback(mpv->opengl_ctx, func, data);
	}
}

void mpv_check_error(int status)
{
	void *array[10];
	size_t size;

	if(status < 0)
	{
		size = (size_t)backtrace(array, 10);

		g_critical("MPV API error: %s\n", mpv_error_string(status));

		backtrace_symbols_fd(array, (int)size, STDERR_FILENO);

		exit(EXIT_FAILURE);
	}
}

inline gboolean gmpv_mpv_obj_is_loaded(GmpvMpvObj *mpv)
{
	return mpv->state.loaded;
}

inline void gmpv_mpv_obj_get_state(GmpvMpvObj *mpv, GmpvMpvObjState *state)
{
	memcpy(state, &mpv->state, sizeof(GmpvMpvObjState));
}

inline gdouble gmpv_mpv_obj_get_autofit_ratio(GmpvMpvObj *mpv)
{
	return mpv->autofit_ratio;
}

inline GmpvPlaylist *gmpv_mpv_obj_get_playlist(GmpvMpvObj *mpv)
{
	return mpv->playlist;
}

inline mpv_handle *gmpv_mpv_obj_get_mpv_handle(GmpvMpvObj *mpv)
{
	return mpv->mpv_ctx;
}

inline mpv_opengl_cb_context *gmpv_mpv_obj_get_opengl_cb_context(GmpvMpvObj *mpv)
{
	return mpv->opengl_ctx;
}

void gmpv_mpv_obj_initialize(GmpvMpvObj *mpv)
{
	GSettings *main_settings = g_settings_new(CONFIG_ROOT);
	GSettings *win_settings = g_settings_new(CONFIG_WIN_STATE);
	gdouble volume = g_settings_get_double(win_settings, "volume")*100;
	gchar *config_dir = get_config_dir_path();
	gchar *mpvopt = NULL;
	gchar *current_vo = NULL;

	const struct
	{
		const gchar *name;
		const gchar *value;
	}
	options[] = {	{"osd-level", "1"},
			{"softvol", "yes"},
			{"force-window", "yes"},
			{"input-default-bindings", "yes"},
			{"audio-client-name", ICON_NAME},
			{"title", "${media-title}"},
			{"autofit-larger", "75%"},
			{"window-scale", "1"},
			{"pause", "no"},
			{"ytdl", "yes"},
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

	g_debug("Setting volume to %f", volume);
	mpv_set_option(mpv->mpv_ctx, "volume", MPV_FORMAT_DOUBLE, &volume);

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
	if(mpv_obj_apply_args(mpv->mpv_ctx, mpvopt) < 0)
	{
		const gchar *msg
			= _("Failed to apply one or more MPV options.");

		g_signal_emit_by_name(mpv, "mpv-error", g_strdup(msg));
	}

	if(mpv->force_opengl)
	{
		g_info("Forcing --vo=opengl-cb");
		mpv_set_option_string(mpv->mpv_ctx, "vo", "opengl-cb");

	}
	else
	{
		g_debug(	"Attaching mpv window to wid %#x",
				(guint)mpv->wid );

		mpv_set_option(	mpv->mpv_ctx,
				"wid",
				MPV_FORMAT_INT64,
				&mpv->wid );
	}

	mpv_observe_property(mpv->mpv_ctx, 0, "aid", MPV_FORMAT_INT64);
	mpv_observe_property(mpv->mpv_ctx, 0, "chapters", MPV_FORMAT_INT64);
	mpv_observe_property(mpv->mpv_ctx, 0, "core-idle", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv->mpv_ctx, 0, "pause", MPV_FORMAT_FLAG);
	mpv_observe_property(mpv->mpv_ctx, 0, "length", MPV_FORMAT_DOUBLE);
	mpv_observe_property(mpv->mpv_ctx, 0, "media-title", MPV_FORMAT_STRING);
	mpv_observe_property(mpv->mpv_ctx, 0, "playlist-pos", MPV_FORMAT_INT64);
	mpv_observe_property(mpv->mpv_ctx, 0, "track-list", MPV_FORMAT_NODE);
	mpv_observe_property(mpv->mpv_ctx, 0, "volume", MPV_FORMAT_DOUBLE);
	mpv_set_wakeup_callback(mpv->mpv_ctx, wakeup_callback, mpv);
	mpv_check_error(mpv_initialize(mpv->mpv_ctx));

	current_vo = gmpv_mpv_obj_get_property_string(mpv, "current-vo");

	if(current_vo && !GDK_IS_X11_DISPLAY(gdk_display_get_default()))
	{
		g_info(	"The chosen vo is %s but the display is not X11; "
			"forcing --vo=opengl-cb and resetting",
			current_vo );

		mpv->force_opengl = TRUE;
		mpv->state.paused = FALSE;

		mpv_free(current_vo);
		gmpv_mpv_obj_reset(mpv);
	}
	else
	{
		/* The vo should be opengl-cb if current_vo is NULL*/
		if(!current_vo)
		{
			mpv->opengl_ctx =	mpv_get_sub_api
						(	mpv->mpv_ctx,
							MPV_SUB_API_OPENGL_CB );
		}

		gmpv_mpv_opt_handle_msg_level(mpv);

		mpv->force_opengl = FALSE;
		mpv->state.ready = TRUE;
		g_signal_emit_by_name(mpv, "mpv-init");
	}

	g_clear_object(&main_settings);
	g_clear_object(&win_settings);
	g_free(config_dir);
	g_free(mpvopt);
}

void gmpv_mpv_obj_reset(GmpvMpvObj *mpv)
{
	const gchar *quit_cmd[] = {"quit_watch_later", NULL};
	gchar *loop_str;
	gboolean loop;
	gint64 playlist_pos;
	gint playlist_pos_rc;

	g_assert(mpv->mpv_ctx);

	loop_str = gmpv_mpv_obj_get_property_string(mpv, "loop");
	loop = (g_strcmp0(loop_str, "inf") == 0);

	mpv_free(loop_str);

	playlist_pos_rc = mpv_get_property(	mpv->mpv_ctx,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );

	/* Reset mpv->mpv_ctx */
	mpv->state.ready = FALSE;

	mpv_check_error(gmpv_mpv_obj_command(mpv, quit_cmd));
	gmpv_mpv_obj_quit(mpv);

	mpv->mpv_ctx = mpv_create();
	gmpv_mpv_obj_initialize(mpv);

	gmpv_mpv_obj_set_event_callback
		(	mpv,
			mpv->event_callback,
			mpv->event_callback_data );
	gmpv_mpv_obj_set_opengl_cb_callback
		(	mpv,
			mpv->opengl_cb_callback,
			mpv->opengl_cb_callback_data );

	gmpv_mpv_obj_set_property_string(	mpv,
					"loop",
					loop?"inf":"no" );

	if(mpv->playlist)
	{
		if(mpv->state.loaded)
		{
			gint rc;

			rc =	mpv_request_event
				(mpv->mpv_ctx, MPV_EVENT_FILE_LOADED, 0);
			mpv_check_error(rc);

			gmpv_mpv_obj_load(mpv, NULL, FALSE, TRUE);

			rc =	mpv_request_event
				(mpv->mpv_ctx, MPV_EVENT_FILE_LOADED, 1);
			mpv_check_error(rc);
		}

		if(playlist_pos_rc >= 0 && playlist_pos > 0)
		{
			gmpv_mpv_obj_set_property(	mpv,
						"playlist-pos",
						MPV_FORMAT_INT64,
						&playlist_pos );
		}

		gmpv_mpv_obj_set_property(	mpv,
					"pause",
					MPV_FORMAT_FLAG,
					&mpv->state.paused );
	}
}

void gmpv_mpv_obj_quit(GmpvMpvObj *mpv)
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

void gmpv_mpv_obj_load(	GmpvMpvObj *mpv,
			const gchar *uri,
			gboolean append,
			gboolean update )
{
	const gchar *load_cmd[] = {"loadfile", NULL, NULL, NULL};
	GtkListStore *playlist_store = gmpv_playlist_get_store(mpv->playlist);
	GtkTreeIter iter;
	gboolean empty;

	g_info(	"Loading file (append=%s, update=%s): %s",
		append?"TRUE":"FALSE",
		update?"TRUE":"FALSE",
		uri?:"<PLAYLIST_ITEMS>" );

	empty = !gtk_tree_model_get_iter_first
			(GTK_TREE_MODEL(playlist_store), &iter);

	load_cmd[2] = (append && !empty)?"append":"replace";

	if(!append && uri && update)
	{
		gmpv_playlist_clear(mpv->playlist);

		mpv->state.new_file = TRUE;
		mpv->state.loaded = FALSE;
	}

	if(!uri)
	{
		gboolean append = FALSE;
		gboolean rc;

		if(!mpv->state.init_load)
		{
			gmpv_mpv_obj_set_property_flag(mpv, "pause", FALSE);
		}

		rc = gtk_tree_model_get_iter_first
			(GTK_TREE_MODEL(playlist_store), &iter);

		while(rc)
		{
			gchar *uri;

			gtk_tree_model_get(	GTK_TREE_MODEL(playlist_store),
						&iter,
						PLAYLIST_URI_COLUMN,
						&uri,
						-1 );

			/* append = FALSE only on first iteration */
			gmpv_mpv_obj_load(mpv, uri, append, FALSE);

			append = TRUE;

			rc = gtk_tree_model_iter_next
				(GTK_TREE_MODEL(playlist_store), &iter);

			g_free(uri);
		}
	}

	if(uri && playlist_store)
	{
		gchar *path = get_path_from_uri(uri);

		load_cmd[1] = path;

		if(!append)
		{
			mpv->state.loaded = FALSE;

			if(!mpv->state.init_load)
			{
				gmpv_mpv_obj_set_property_flag
					(mpv, "pause", FALSE);
			}
		}

		if(update)
		{
			gchar *name = get_name_from_path(path);

			gmpv_playlist_append(mpv->playlist, name, uri);

			g_free(name);
		}

		g_assert(mpv->mpv_ctx);

		mpv_check_error(mpv_request_event(	mpv->mpv_ctx,
							MPV_EVENT_END_FILE,
							0 ));

		mpv_check_error(mpv_command(mpv->mpv_ctx, load_cmd));

		mpv_check_error(mpv_request_event(	mpv->mpv_ctx,
							MPV_EVENT_END_FILE,
							1 ));

		g_free(path);
	}
}

void gmpv_mpv_obj_load_list(	GmpvMpvObj *mpv,
			const gchar **uri_list,
			gboolean append,
			gboolean update )
{
	static const char *const sub_exts[]
		= {	"utf", "utf8", "utf-8", "idx", "sub", "srt", "smi",
			"rt", "txt", "ssa", "aqt", "jss", "js", "ass", "mks",
			"vtt", "sup", NULL };

	for(gint i = 0; uri_list[i]; i++)
	{
		const gchar *ext = strrchr(uri_list[i], '.');
		gint j;

		/* Only start checking the extension if there is at
		 * least one character after the dot.
		 */
		if(ext && ++ext)
		{
			for(	j = 0;
				sub_exts[j] &&
				g_strcmp0(ext, sub_exts[j]) != 0;
				j++ );
		}

		/* Only attempt to load file as subtitle if there
		 * already is a file loaded. Try to load the file as a
		 * media file otherwise.
		 */
		if(ext && sub_exts[j] && gmpv_mpv_obj_is_loaded(mpv))
		{
			const gchar *cmd[] = {"sub-add", NULL, NULL};
			gchar *path;

			/* Convert to path if possible to get rid of
			 * percent encoding.
			 */
			path =	g_filename_from_uri
				(uri_list[i], NULL, NULL);

			cmd[1] = path?:uri_list[i];

			gmpv_mpv_obj_command(mpv, cmd);

			g_free(path);
		}
		else
		{
			gboolean empty = gmpv_playlist_empty(mpv->playlist);

			gmpv_mpv_obj_load(	mpv,
						uri_list[i],
						((append && !empty) || i != 0),
						TRUE );
		}
	}
}
