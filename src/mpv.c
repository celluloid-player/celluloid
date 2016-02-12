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
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

#include "mpv.h"
#include "def.h"
#include "track.h"
#include "playlist.h"
#include "control_box.h"
#include "playlist_widget.h"

static void parse_dim_string(	gmpv_handle *ctx,
				const gchar *mpv_geom_str,
				gint *width,
				gint *height );
static void handle_autofit_opt(gmpv_handle *ctx);
static void handle_msg_level_opt(gmpv_handle *ctx);
static void handle_property_change_event(	gmpv_handle *ctx,
						mpv_event_property* prop);
static void opengl_callback(void *cb_ctx);
static void uninit_opengl_cb(gmpv_handle *ctx);
static Track *parse_track_list(mpv_node_list *node);

static void parse_dim_string(	gmpv_handle *ctx,
				const gchar *mpv_geom_str,
				gint *width,
				gint *height )
{
	GdkScreen *screen = NULL;
	gint screen_width = -1;
	gint screen_height = -1;
	gchar **tokens = NULL;
	gint i = -1;

	screen = gdk_screen_get_default();
	screen_width = gdk_screen_get_width(screen);
	screen_height = gdk_screen_get_height(screen);
	tokens = g_strsplit(mpv_geom_str, "x", 2);

	*width = 0;
	*height = 0;

	while(tokens && tokens[++i] && i < 4)
	{
		gdouble multiplier = -1;
		gint value = -1;

		value = (gint)g_ascii_strtoll(tokens[i], NULL, 0);

		if(tokens[i][strnlen(tokens[i], 256)-1] == '%')
		{
			multiplier = value/100.0;
		}

		if(i == 0)
		{
			if(multiplier == -1)
			{
				*width = value;
			}
			else
			{
				*width = (gint)(multiplier*screen_width);
			}
		}
		else if(i == 1)
		{
			if(multiplier == -1)
			{
				*height = value;
			}
			else
			{
				*height = (gint)(multiplier*screen_height);
			}
		}
	}

	if(*width != 0 && *height == 0)
	{
		/* If height is not specified, set it to screen height. This
		 * should correctly emulate vanilla mpv's behavior since we
		 * always preserve aspect ratio when autofitting.
		 */
		*height = screen_height;
	}

	g_strfreev(tokens);
}

static void handle_autofit_opt(gmpv_handle *ctx)
{
	gchar *optbuf = NULL;
	gchar *geom = NULL;

	optbuf = mpv_get_property_string(ctx->mpv_ctx, "options/autofit");

	if(optbuf && strlen(optbuf) > 0)
	{
		gint autofit_width = 0;
		gint autofit_height = 0;
		gint64 vid_width = 0;
		gint64 vid_height = 0;
		gdouble width_ratio = -1;
		gdouble height_ratio = -1;
		gint rc = 0;

		rc |= mpv_get_property(	ctx->mpv_ctx,
					"dwidth",
					MPV_FORMAT_INT64,
					&vid_width );

		rc |= mpv_get_property(	ctx->mpv_ctx,
					"dheight",
					MPV_FORMAT_INT64,
					&vid_height );

		if(rc >= 0)
		{
			parse_dim_string(	ctx,
						optbuf,
						&autofit_width,
						&autofit_height );

			width_ratio =	(gdouble)autofit_width/
					(gdouble)vid_width;

			height_ratio =	(gdouble)autofit_height/
					(gdouble)vid_height;
		}

		if(rc >= 0 && width_ratio > 0 && height_ratio > 0)
		{
			if(width_ratio > 1 && height_ratio > 1)
			{
				resize_window_to_fit(ctx, 1);
			}
			else
			{
				/* Resize the window so that it is as big as
				 * possible within the limits imposed by
				 * 'autofit' while preseving the aspect ratio.
				 */
				resize_window_to_fit
					(	ctx,
						(width_ratio < height_ratio)
						?width_ratio
						:height_ratio );
			}
		}
	}

	mpv_free(optbuf);
	g_free(geom);
}

static void handle_msg_level_opt(gmpv_handle *ctx)
{
	const struct
	{
		gchar *name;
		mpv_log_level level;
	}
	level_map[] = {	{"no", MPV_LOG_LEVEL_NONE},
			{"fatal", MPV_LOG_LEVEL_FATAL},
			{"error", MPV_LOG_LEVEL_ERROR},
			{"warn", MPV_LOG_LEVEL_WARN},
			{"info", MPV_LOG_LEVEL_INFO},
			{"v", MPV_LOG_LEVEL_V},
			{"debug", MPV_LOG_LEVEL_DEBUG},
			{"trace", MPV_LOG_LEVEL_TRACE},
			{NULL, MPV_LOG_LEVEL_NONE} };

	gchar *optbuf = NULL;
	gchar **tokens = NULL;
	mpv_log_level min_level = DEFAULT_LOG_LEVEL;
	gint i;

	optbuf = mpv_get_property_string(ctx->mpv_ctx, "options/msg-level");

	if(optbuf)
	{
		tokens = g_strsplit(optbuf, ",", 0);
	}

	if(ctx->log_level_list)
	{
		g_slist_free_full(ctx->log_level_list, g_free);

		ctx->log_level_list = NULL;
	}

	for(i = 0; tokens && tokens[i]; i++)
	{
		gchar **pair = g_strsplit(tokens[i], "=", 2);
		module_log_level *level = g_malloc(sizeof(module_log_level));
		gboolean found = FALSE;
		gint j;

		level->prefix = g_strdup(pair[0]);

		for(j = 0; level_map[j].name && !found; j++)
		{
			if(g_strcmp0(pair[1], level_map[j].name) == 0)
			{
				level->level = level_map[j].level;
				found = TRUE;
			}
		}


		/* Ignore if the given level is invalid */
		if(found)
		{
			/* Lower log levels have higher values */
			if(level->level > min_level)
			{
				min_level = level->level;
			}

			if(g_strcmp0(level->prefix, "all") != 0)
			{
				ctx->log_level_list
					= g_slist_append
						(ctx->log_level_list, level);
			}
		}

		g_strfreev(pair);
	}

	for(i = 0; level_map[i].level != min_level; i++);

	mpv_check_error
		(mpv_request_log_messages(ctx->mpv_ctx, level_map[i].name));

	mpv_free(optbuf);
	g_strfreev(tokens);
}

static void handle_property_change_event(	gmpv_handle *ctx,
						mpv_event_property* prop)
{
	if(g_strcmp0(prop->name, "pause") == 0)
	{
		GtkWindow *wnd = GTK_WINDOW(ctx->gui);
		GtkApplication *app = gtk_window_get_application(wnd);
		gboolean idle;

		ctx->paused = prop->data?*((int *)prop->data):TRUE;

		mpv_get_property(ctx->mpv_ctx, "idle", MPV_FORMAT_FLAG, &idle);

		if(idle && !ctx->paused)
		{
			mpv_load(ctx, NULL, FALSE, TRUE);
		}

		if(!ctx->paused)
		{
			ctx->inhibit_cookie
				= gtk_application_inhibit
					(	app, wnd,
						GTK_APPLICATION_INHIBIT_IDLE,
						_("Playing") );
		}
		else if(ctx->inhibit_cookie != 0)
		{
			gtk_application_uninhibit(app, ctx->inhibit_cookie);
		}

		mpv_load_gui_update(ctx);
	}
	else if(g_strcmp0(prop->name, "volume") == 0
	&& (ctx->init_load || ctx->loaded))
	{
		ControlBox *control_box = CONTROL_BOX(ctx->gui->control_box);
		gdouble volume = prop->data?*((double *)prop->data)/100.0:0;

		g_signal_handlers_block_matched
			(	control_box->volume_button,
				G_SIGNAL_MATCH_DATA,
				0,
				0,
				NULL,
				NULL,
				ctx );

		control_box_set_volume(control_box, volume);

		g_signal_handlers_unblock_matched
			(	control_box->volume_button,
				G_SIGNAL_MATCH_DATA,
				0,
				0,
				NULL,
				NULL,
				ctx );
	}
	else if(g_strcmp0(prop->name, "aid") == 0)
	{
		ControlBox *control_box = CONTROL_BOX(ctx->gui->control_box);

		/* prop->data == NULL iff there is no audio track */
		gtk_widget_set_sensitive
			(control_box->volume_button, !!prop->data);
	}
	else if(g_strcmp0(prop->name, "fullscreen") == 0)
	{
		int *data = prop->data;
		int fullscreen = data?*data:-1;

		if(fullscreen != -1 && fullscreen != ctx->gui->fullscreen)
		{
			main_window_toggle_fullscreen(MAIN_WINDOW(ctx->gui));
		}
	}
	else if(g_strcmp0(prop->name, "eof-reached") == 0
	&& prop->data
	&& *((int *)prop->data) == 1)
	{
		ctx->paused = TRUE;
		ctx->loaded = FALSE;

		main_window_reset(ctx->gui);
		playlist_reset(ctx);
	}
}

static void opengl_callback(void *cb_ctx)
{
#ifdef OPENGL_CB_ENABLED
	gmpv_handle *ctx = cb_ctx;

	if(ctx->opengl_ctx)
	{
		gtk_gl_area_queue_render(GTK_GL_AREA(ctx->gui->vid_area));
	}
#endif
}

static void uninit_opengl_cb(gmpv_handle *ctx)
{
#ifdef OPENGL_CB_ENABLED
	gtk_gl_area_make_current(GTK_GL_AREA(ctx->gui->vid_area));
	mpv_opengl_cb_uninit_gl(ctx->opengl_ctx);
#endif
}

static Track *parse_track_list(mpv_node_list *node)
{
	Track *entry = track_new();

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

void mpv_wakeup_callback(void *data)
{
	g_idle_add((GSourceFunc)mpv_handle_event, data);
}

void mpv_log_handler(gmpv_handle *ctx, mpv_event_log_message* message)
{
	GSList *iter = ctx->log_level_list;
	module_log_level *level = NULL;
	gsize event_prefix_len = strlen(message->prefix);
	gboolean found = FALSE;

	if(iter)
	{
		level = iter->data;
	}

	while(iter && !found)
	{
		gsize prefix_len;
		gint cmp;

		prefix_len = strlen(level->prefix);

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

gboolean mpv_handle_event(gpointer data)
{
	gmpv_handle *ctx = data;
	mpv_event *event = NULL;
	gboolean done = FALSE;

	while(!done)
	{
		event = ctx->mpv_ctx?mpv_wait_event(ctx->mpv_ctx, 0):NULL;

		if(!event)
		{
			done = TRUE;
		}
		else if(event->event_id == MPV_EVENT_PROPERTY_CHANGE)
		{
			mpv_event_property *prop = event->data;

			handle_property_change_event(ctx, prop);

			g_signal_emit_by_name(	ctx->gui,
						"mpv-prop-change",
						g_strdup(prop->name) );
		}
		else if(event->event_id == MPV_EVENT_IDLE)
		{
			if(ctx->init_load)
			{
				mpv_load(ctx, NULL, FALSE, FALSE);
			}
			else if(ctx->loaded)
			{
				gint rc;

				ctx->paused = TRUE;
				ctx->loaded = FALSE;

				rc = mpv_set_property(	ctx->mpv_ctx,
							"pause",
							MPV_FORMAT_FLAG,
							&ctx->paused );

				mpv_check_error(rc);
				main_window_reset(ctx->gui);
				playlist_reset(ctx);
			}

			ctx->init_load = FALSE;
		}
		else if(event->event_id == MPV_EVENT_FILE_LOADED)
		{
			ctx->loaded = TRUE;
			ctx->init_load = FALSE;

			mpv_update_playlist(ctx);
			mpv_load_gui_update(ctx);
		}
		else if(event->event_id == MPV_EVENT_END_FILE)
		{
			mpv_event_end_file *ef_event = event->data;

			ctx->init_load = FALSE;

			if(ctx->loaded)
			{
				ctx->new_file = FALSE;
			}

			if(ef_event->reason == MPV_END_FILE_REASON_ERROR)
			{
				const gchar *err_str;
				gchar *msg;

				err_str = mpv_error_string(ef_event->error);

				msg = g_strdup_printf
					(	_("Playback was terminated "
						"abnormally. Reason: %s."),
						err_str );

				show_error_dialog(ctx, NULL, msg);

				g_free(msg);
			}
		}
		else if(event->event_id == MPV_EVENT_VIDEO_RECONFIG)
		{
			if(ctx->new_file)
			{
				handle_autofit_opt(ctx);
			}
		}
		else if(event->event_id == MPV_EVENT_PLAYBACK_RESTART)
		{
			mpv_load_gui_update(ctx);

			g_signal_emit_by_name(	ctx->gui,
						"mpv-playback-restart" );
		}
		else if(event->event_id == MPV_EVENT_SHUTDOWN)
		{
			quit(ctx);

			done = TRUE;
		}
		else if(event->event_id == MPV_EVENT_LOG_MESSAGE)
		{
			mpv_log_handler(ctx, event->data);
		}
		else if(event->event_id == MPV_EVENT_NONE)
		{
			done = TRUE;
		}
	}

	return FALSE;
}

void mpv_update_playlist(gmpv_handle *ctx)
{
	/* The length of "playlist//filename" including null-terminator (19)
	 * plus the number of digits in the maximum value of 64 bit int (19).
	 */
	const gsize filename_prop_str_size = 38;
	PlaylistWidget *playlist;
	GtkListStore *store;
	GtkTreeIter iter;
	mpv_node mpv_playlist;
	gchar *filename_prop_str;
	gboolean iter_end;
	gint playlist_count;
	gint i;

	playlist = PLAYLIST_WIDGET(ctx->gui->playlist);
	store = playlist->list_store;
	filename_prop_str = g_malloc(filename_prop_str_size);
	iter_end = FALSE;

	mpv_check_error(mpv_get_property(	ctx->mpv_ctx,
						"playlist",
						MPV_FORMAT_NODE,
						&mpv_playlist ));

	playlist_count = mpv_playlist.u.list->num;

	g_signal_handlers_block_matched
		(	playlist->list_store,
			G_SIGNAL_MATCH_DATA,
			0,
			0,
			NULL,
			NULL,
			ctx );

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	for(i = 0; i < playlist_count; i++)
	{
		gint prop_count = 0;
		gchar *uri = NULL;
		gchar *title = NULL;
		gchar *name = NULL;
		gchar *old_name = NULL;
		gchar *old_uri = NULL;

		prop_count = mpv_playlist.u.list->values[i].u.list->num;

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

		name = title?title:get_name_from_path(uri);

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
			playlist_widget_append(playlist, name, uri);
		}

		mpv_free(uri);
		g_free(name);
	}

	/* If there are more entries in the playlist widget than mpv's playlist,
	 * remove the excess entries from the playlist widget.
	 */
	if(!iter_end)
	{
		while(gtk_list_store_remove(store, &iter));
	}

	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(playlist->tree_view));

	g_signal_handlers_unblock_matched
		(	playlist->list_store,
			G_SIGNAL_MATCH_DATA,
			0,
			0,
			NULL,
			NULL,
			ctx );

	g_free(filename_prop_str);
	mpv_free_node_contents(&mpv_playlist);
}

void mpv_load_gui_update(gmpv_handle *ctx)
{
	ControlBox *control_box;
	mpv_node track_list;
	gchar* title;
	gint64 chapter_count;
	gint64 playlist_pos;
	gdouble length;
	gdouble volume;

	control_box = CONTROL_BOX(ctx->gui->control_box);
	title = mpv_get_property_string(ctx->mpv_ctx, "media-title");

	if(title)
	{
		gtk_window_set_title(GTK_WINDOW(ctx->gui), title);

		mpv_free(title);
	}

	mpv_check_error(mpv_set_property(	ctx->mpv_ctx,
						"pause",
						MPV_FORMAT_FLAG,
						&ctx->paused));

	if(mpv_get_property(	ctx->mpv_ctx,
				"track-list",
				MPV_FORMAT_NODE,
				&track_list) >= 0)
	{
		mpv_node_list *org_list = track_list.u.list;
		GSList *audio_list = NULL;
		GSList *video_list = NULL;
		GSList *sub_list = NULL;
		GAction *action = NULL;
		gint64 aid = -1;
		gint64 sid = -1;

		mpv_get_property(	ctx->mpv_ctx,
					"aid",
					MPV_FORMAT_INT64,
					&aid );

		mpv_get_property(	ctx->mpv_ctx,
					"sid",
					MPV_FORMAT_INT64,
					&sid );

		action = g_action_map_lookup_action
				(G_ACTION_MAP(ctx->app), "audio_select");

		g_simple_action_set_state
			(G_SIMPLE_ACTION(action), g_variant_new_int64(aid));

		action = g_action_map_lookup_action
				(G_ACTION_MAP(ctx->app), "sub_select");

		g_simple_action_set_state
			(G_SIMPLE_ACTION(action), g_variant_new_int64(sid));

		for(gint i = 0; i < org_list->num; i++)
		{
			Track *entry;
			GSList **list;

			entry = parse_track_list
				(org_list->values[i].u.list);

			if(entry->type == TRACK_TYPE_AUDIO)
			{
				list = &audio_list;
			}
			else if(entry->type == TRACK_TYPE_VIDEO)
			{
				list = &video_list;
			}
			else if(entry->type == TRACK_TYPE_SUBTITLE)
			{
				list = &sub_list;
			}
			else
			{
				g_assert(FALSE);
			}

			*list = g_slist_prepend(*list, entry);
		}

		audio_list = g_slist_reverse(audio_list);
		video_list = g_slist_reverse(video_list);
		sub_list = g_slist_reverse(sub_list);

		main_window_update_track_list
			(ctx->gui, audio_list, video_list, sub_list);

		g_slist_free_full
			(audio_list, (GDestroyNotify)track_free);

		g_slist_free_full
			(video_list, (GDestroyNotify)track_free);

		g_slist_free_full
			(sub_list, (GDestroyNotify)track_free);
	}

	if(mpv_get_property(	ctx->mpv_ctx,
				"playlist-pos",
				MPV_FORMAT_INT64,
				&playlist_pos) >= 0)
	{
		playlist_widget_set_indicator_pos
			(	PLAYLIST_WIDGET(ctx->gui->playlist),
				(gint)playlist_pos );
	}

	if(mpv_get_property(	ctx->mpv_ctx,
				"chapters",
				MPV_FORMAT_INT64,
				&chapter_count) >= 0)
	{
		control_box_set_chapter_enabled(	control_box,
							(chapter_count > 1) );
	}

	if(mpv_get_property(	ctx->mpv_ctx,
				"volume",
				MPV_FORMAT_DOUBLE,
				&volume) >= 0)
	{
		control_box_set_volume(control_box, volume/100);
	}

	if(mpv_get_property(	ctx->mpv_ctx,
				"length",
				MPV_FORMAT_DOUBLE,
				&length) >= 0)
	{
		control_box_set_seek_bar_length(control_box, (gint)length);
	}

	control_box_set_playing_state(control_box, !ctx->paused);
}

gint mpv_apply_args(mpv_handle *mpv_ctx, gchar *args)
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

void mpv_init(gmpv_handle *ctx)
{
	GSettings *settings = g_settings_new(CONFIG_WIN_STATE);
	gdouble volume = g_settings_get_double(settings, "volume")*100;
	gchar *config_dir = get_config_dir_path();
	gchar *mpvopt = NULL;

	const struct
	{
		const gchar *name;
		const gchar *value;
	}
	options[] = {	{"osd-level", "1"},
			{"softvol", "yes"},
			{"force-window", "yes"},
			{"audio-client-name", ICON_NAME},
			{"title", "${media-title}"},
			{"pause", "yes"},
			{"ytdl", "yes"},
			{"input-cursor", "no"},
			{"cursor-autohide", "no"},
			{"softvol-max", "100"},
			{"config", "yes"},
			{"screenshot-template", "gnome-mpv-shot%n"},
			{"config-dir", config_dir},
			{NULL, NULL} };

	for(gint i = 0; options[i].name; i++)
	{
		g_debug(	"Applying default option --%s=%s",
				options[i].name,
				options[i].value );

		mpv_set_option_string(	ctx->mpv_ctx,
					options[i].name,
					options[i].value );
	}

	g_debug("Setting volume to %f", volume);
	mpv_set_option(ctx->mpv_ctx, "volume", MPV_FORMAT_DOUBLE, &volume);

	if(g_settings_get_boolean(ctx->config, "mpv-config-enable"))
	{
		gchar *mpv_conf = g_settings_get_string
					(ctx->config, "mpv-config-file");

		g_info("Loading config file: %s", mpv_conf);
		mpv_load_config_file(ctx->mpv_ctx, mpv_conf);

		g_free(mpv_conf);
	}

	mpvopt = g_settings_get_string(ctx->config, "mpv-options");

	g_debug("Applying extra mpv options: %s", mpvopt);

	/* Apply extra options */
	if(mpv_apply_args(ctx->mpv_ctx, mpvopt) < 0)
	{
		const gchar *msg
			= _("Failed to apply one or more MPV options.");

		show_error_dialog(ctx, NULL, msg);
	}

	if(main_window_get_use_opengl(ctx->gui))
	{
		g_info("opengl-cb is enabled; forcing --vo=opengl-cb");
		mpv_set_option_string(ctx->mpv_ctx, "vo", "opengl-cb");

	}
	else
	{
		g_debug(	"Attaching mpv window to wid %#x",
				(guint)ctx->vid_area_wid );

		mpv_set_option(	ctx->mpv_ctx,
				"wid",
				MPV_FORMAT_INT64,
				&ctx->vid_area_wid );
	}

	mpv_observe_property(ctx->mpv_ctx, 0, "aid", MPV_FORMAT_INT64);
	mpv_observe_property(ctx->mpv_ctx, 0, "pause", MPV_FORMAT_FLAG);
	mpv_observe_property(ctx->mpv_ctx, 0, "eof-reached", MPV_FORMAT_FLAG);
	mpv_observe_property(ctx->mpv_ctx, 0, "fullscreen", MPV_FORMAT_FLAG);
	mpv_observe_property(ctx->mpv_ctx, 0, "volume", MPV_FORMAT_DOUBLE);
	mpv_check_error(mpv_initialize(ctx->mpv_ctx));

	ctx->opengl_ctx = mpv_get_sub_api(ctx->mpv_ctx, MPV_SUB_API_OPENGL_CB);

	mpv_opengl_cb_set_update_callback(	ctx->opengl_ctx,
						opengl_callback,
						ctx );

	handle_msg_level_opt(ctx);
	g_signal_emit_by_name(ctx->gui, "mpv-init");

	g_clear_object(&settings);
	g_free(config_dir);
	g_free(mpvopt);
}

void mpv_quit(gmpv_handle *ctx)
{
	g_info("Terminating mpv");

	if(gtk_widget_get_realized(ctx->gui->vid_area)
	&& main_window_get_use_opengl(ctx->gui))
	{
		g_debug("Uninitializing opengl-cb");
		uninit_opengl_cb(ctx);

		ctx->opengl_ready = FALSE;
	}

	mpv_terminate_destroy(ctx->mpv_ctx);
}

void mpv_load(	gmpv_handle *ctx,
		const gchar *uri,
		gboolean append,
		gboolean update )
{
	const gchar *load_cmd[] = {"loadfile", NULL, NULL, NULL};
	GtkListStore *playlist_store;
	GtkTreeIter iter;
	gboolean empty;

	g_info(	"Loading file (append=%s, update=%s): %s",
		append?"TRUE":"FALSE",
		update?"TRUE":"FALSE",
		uri?:"<PLAYLIST_ITEMS>" );

	playlist_store = GTK_LIST_STORE(ctx->playlist_store);

	empty = !gtk_tree_model_get_iter_first
			(GTK_TREE_MODEL(playlist_store), &iter);

	load_cmd[2] = (append && !empty)?"append":"replace";

	if(!append && uri && update)
	{
		playlist_widget_clear
			(PLAYLIST_WIDGET(ctx->gui->playlist));

		ctx->new_file = TRUE;
		ctx->loaded = FALSE;
	}

	if(!uri)
	{
		gboolean rc;
		gboolean append;

		rc = gtk_tree_model_get_iter_first
			(GTK_TREE_MODEL(playlist_store), &iter);

		append = FALSE;

		while(rc)
		{
			gchar *uri;

			gtk_tree_model_get(	GTK_TREE_MODEL(playlist_store),
						&iter,
						PLAYLIST_URI_COLUMN,
						&uri,
						-1 );

			/* append = FALSE only on first iteration */
			mpv_load(ctx, uri, append, FALSE);

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
			ctx->loaded = FALSE;
		}

		if(update)
		{
			gchar *name = get_name_from_path(path);

			playlist_widget_append
				(	PLAYLIST_WIDGET(ctx->gui->playlist),
					name,
					uri );

			g_free(name);
		}

		control_box_set_enabled
			(CONTROL_BOX(ctx->gui->control_box), TRUE);

		mpv_check_error(mpv_request_event(	ctx->mpv_ctx,
							MPV_EVENT_END_FILE,
							0 ));

		mpv_check_error(mpv_command(ctx->mpv_ctx, load_cmd));

		mpv_check_error(mpv_set_property(	ctx->mpv_ctx,
							"pause",
							MPV_FORMAT_FLAG,
							&ctx->paused ));

		mpv_check_error(mpv_request_event(	ctx->mpv_ctx,
							MPV_EVENT_END_FILE,
							1 ));

		g_free(path);
	}
}
