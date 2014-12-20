/*
 * Copyright (c) 2014 gnome-mpv
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

#include "mpv.h"
#include "playlist.h"
#include "control_box.h"
#include "playlist_widget.h"

void *mpv_event_handler(void *data)
{
	gmpv_handle *ctx = data;
	gint exit_flag = FALSE;
	mpv_event *event;

	while(!exit_flag)
	{
		event = mpv_wait_event(ctx->mpv_ctx, 1);

		if(event->event_id == MPV_EVENT_PROPERTY_CHANGE)
		{
			mpv_event_property *prop = event->data;

			if(g_strcmp0(prop->name, "pause") == 0)
			{
				pthread_mutex_lock(ctx->mpv_event_mutex);

				ctx->paused = *((int *)prop->data);

				pthread_mutex_unlock(ctx->mpv_event_mutex);

				g_idle_add(	(GSourceFunc)
						mpv_load_gui_update,
						ctx );
			}
			else if(g_strcmp0(prop->name, "eof-reached") == 0
			&& prop->data
			&& *((int *)prop->data) == 1)
			{
				pthread_mutex_lock(ctx->mpv_event_mutex);

				ctx->paused = TRUE;
				ctx->loaded = FALSE;

				g_idle_add((GSourceFunc)control_reset, ctx);
				g_idle_add((GSourceFunc)playlist_reset, ctx);

				pthread_mutex_unlock(ctx->mpv_event_mutex);
			}
		}
		else if(event->event_id == MPV_EVENT_IDLE)
		{
			pthread_mutex_lock(ctx->mpv_event_mutex);

			if(ctx->loaded)
			{
				ctx->paused = TRUE;
				ctx->loaded = FALSE;

				g_idle_add((GSourceFunc)control_reset, ctx);
				g_idle_add((GSourceFunc)playlist_reset, ctx);
			}

			pthread_mutex_unlock(ctx->mpv_event_mutex);
		}
		else if(event->event_id == MPV_EVENT_FILE_LOADED)
		{
			pthread_mutex_lock(ctx->mpv_event_mutex);

			ctx->loaded = TRUE;

			g_idle_add(	(GSourceFunc)
					mpv_load_gui_update,
					ctx );

			pthread_mutex_unlock(ctx->mpv_event_mutex);
		}
		else if(event->event_id == MPV_EVENT_PLAYBACK_RESTART)
		{
			g_idle_add((GSourceFunc)mpv_load_gui_update, ctx);
		}
		else if(event->event_id == MPV_EVENT_SHUTDOWN)
		{
			g_idle_add((GSourceFunc)quit, ctx);
		}
		else if(event->event_id == MPV_EVENT_LOG_MESSAGE)
		{
			mpv_log_handler(ctx, event->data);
		}

		pthread_mutex_lock(ctx->mpv_event_mutex);

		if(ctx->mpv_ctx_reset)
		{
			pthread_cond_signal(ctx->mpv_ctx_destroy_cv);

			pthread_cond_wait(	ctx->mpv_ctx_init_cv,
						ctx->mpv_event_mutex );
		}

		exit_flag = ctx->exit_flag;

		pthread_mutex_unlock(ctx->mpv_event_mutex);
	}

	return NULL;
}

void mpv_log_handler(gmpv_handle *ctx, mpv_event_log_message* message)
{
	const gchar *text = message->text;
	gchar *buffer;
	gboolean message_complete;
	gboolean log_complete;

	pthread_mutex_lock(ctx->mpv_event_mutex);

	buffer = ctx->log_buffer;

	message_complete = (text && text[strlen(text)-1] == '\n');

	log_complete = (buffer && buffer[strlen(buffer)-1] == '\n');

	/* If the buffer is not empty, new log messages will be ignored
	 * until the buffer is cleared by show_error_dialog().
	 */
	if(buffer && !log_complete)
	{
		ctx->log_buffer = g_strconcat(buffer, text, NULL);

		g_free(buffer);
	}
	else if(!buffer)
	{
		ctx->log_buffer = g_strdup(text);
	}

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	if(!log_complete && message_complete)
	{
		/* ctx->log_buffer will be freed by show_error_dialog().
		 */
		g_idle_add((GSourceFunc)show_error_dialog, ctx);
	}
}

void mpv_check_error(int status)
{
	void *array[10];
	size_t size;

	if(status < 0)
	{
		size = backtrace(array, 10);

		fprintf(	stderr,
				"MPV API error: %s\n",
				mpv_error_string(status) );

		backtrace_symbols_fd(array, size, STDERR_FILENO);

		exit(EXIT_FAILURE);
	}
}

gboolean mpv_load_gui_update(gpointer data)
{
	gmpv_handle* ctx = data;
	ControlBox *control_box;
	gchar* title;
	gint64 chapter_count;
	gint64 playlist_pos;
	gboolean new_file;
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
				"playlist-pos",
				MPV_FORMAT_INT64,
				&playlist_pos) >= 0)
	{
		playlist_widget_set_indicator_pos
			(PLAYLIST_WIDGET(ctx->gui->playlist), playlist_pos);
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
		control_box_set_seek_bar_length(control_box, length);
	}

	pthread_mutex_lock(ctx->mpv_event_mutex);

	new_file = ctx->new_file;
	ctx->new_file = FALSE;

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	control_box_set_playing_state(control_box, !ctx->paused);

	if(new_file)
	{
		resize_window_to_fit(ctx, 1);
	}

	return FALSE;
}

gboolean mpv_load_from_ctx(gpointer data)
{
	gmpv_handle* ctx = data;

	mpv_load(ctx, NULL, FALSE, FALSE);

	return FALSE;
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
		gint token_size;

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

		token_size = opt_end-opt_begin;
		token = g_malloc(token_size);

		strncpy(token, opt_begin+2, token_size-1);

		token[token_size-1] = '\0';
		token_arg = strpbrk(token, "= ");

		if(token_arg)
		{
			*token_arg = '\0';

			token_arg++;
		}
		else
		{
			/* Default to "yes" if option has no explicit argument
			 */
			token_arg = "yes";
		}

		/* Failing to apply extra options is non-fatal */
		if(mpv_set_option_string(mpv_ctx, token, token_arg) < 0)
		{
			fail_count++;

			fprintf(	stderr,
					"Failed to apply option: --%s=%s\n",
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

void mpv_init(gmpv_handle *ctx, gint64 vid_area_wid)
{
	gboolean mpvconf_enable = FALSE;
	gchar *mpvconf = NULL;
	gchar *mpvopt = NULL;
	gchar *screenshot_template = NULL;

	screenshot_template
		= g_build_filename(g_get_home_dir(), "screenshot-%n", NULL);

	/* Set default options */
	mpv_check_error(mpv_set_option_string(ctx->mpv_ctx, "osd-level", "1"));
	mpv_check_error(mpv_set_option_string(ctx->mpv_ctx, "softvol", "yes"));

	mpv_check_error(mpv_set_option_string(	ctx->mpv_ctx,
						"softvol-max",
						"100" ));

	mpv_check_error(mpv_set_option_string(	ctx->mpv_ctx,
						"config",
						"yes" ));

	mpv_check_error(mpv_set_option_string(	ctx->mpv_ctx,
						"screenshot-template",
						screenshot_template ));

	mpv_check_error(mpv_set_option(	ctx->mpv_ctx,
					"wid",
					MPV_FORMAT_INT64,
					&vid_area_wid ));

	mpv_check_error(mpv_observe_property(	ctx->mpv_ctx,
						0,
						"pause",
						MPV_FORMAT_FLAG ));

	mpv_check_error(mpv_observe_property(	ctx->mpv_ctx,
						0,
						"eof-reached",
						MPV_FORMAT_FLAG ));

	mpv_check_error(mpv_request_log_messages(ctx->mpv_ctx, "error"));

	mpvconf_enable = get_config_boolean(ctx, "main", "mpv-config-enable");
	mpvconf = get_config_string(ctx, "main", "mpv-config-file");
	mpvopt = get_config_string(ctx, "main", "mpv-options");

	if(mpvconf_enable)
	{
		mpv_load_config_file(ctx->mpv_ctx, mpvconf);
	}

	/* Apply extra options */
	if(mpv_apply_args(ctx->mpv_ctx, mpvopt) < 0)
	{
		ctx->log_buffer
			= g_strdup("Failed to apply one or more MPV options.");

		show_error_dialog(ctx);
	}

	mpv_check_error(mpv_initialize(ctx->mpv_ctx));

	g_free(mpvconf);
	g_free(mpvopt);
	g_free(screenshot_template);
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

	pthread_mutex_lock(ctx->mpv_event_mutex);

	playlist_store = GTK_LIST_STORE(ctx->playlist_store);

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	empty = !gtk_tree_model_get_iter_first
			(GTK_TREE_MODEL(playlist_store), &iter);

	load_cmd[2] = (append && !empty)?"append":"replace";

	if(!append && uri && update)
	{
		playlist_widget_clear
			(PLAYLIST_WIDGET(ctx->gui->playlist));

		pthread_mutex_lock(ctx->mpv_event_mutex);

		ctx->new_file = TRUE;

		pthread_mutex_unlock(ctx->mpv_event_mutex);
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
			pthread_mutex_lock(ctx->mpv_event_mutex);

			ctx->loaded = FALSE;

			pthread_mutex_unlock(ctx->mpv_event_mutex);
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
