/*
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

#include <mpv/client.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gstdio.h>
#include <gio/gvfs.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

#include "def.h"
#include "main_window.h"
#include "playlist_widget.h"
#include "pref_dialog.h"
#include "open_loc_dialog.h"

typedef struct context_t
{
	mpv_handle *mpv_ctx;
	volatile gboolean exit_flag;
	volatile gboolean mpv_ctx_reset;
	gboolean paused;
	gboolean loaded;
	gboolean new_file;
	gboolean sub_visible;
	gint64 vid_area_wid;
	gchar *log_buffer;
	gchar *mpv_options;
	GPtrArray *uri_list;
	GKeyFile *config_file;
	GtkWidget *fs_control;
	MainWindow *gui;
	pthread_t *mpv_event_handler_thread;
	pthread_mutex_t *mpv_event_mutex;
	pthread_cond_t *mpv_ctx_init_cv;
	pthread_cond_t *mpv_ctx_destroy_cv;
}
context_t;

static inline void mpv_check_error(int status);
static inline gchar *get_config_file_path(void);
static gboolean update_seek_bar(gpointer data);
static gboolean reset_control(gpointer data);
static gboolean reset_playlist(gpointer data);
static gboolean show_error_dialog(gpointer data);
static gboolean mpv_load_gui_update(gpointer data);
static gboolean mpv_load_from_ctx(gpointer data);
static gboolean load_config(context_t *ctx);
static gboolean save_config(context_t *ctx);
static gint mpv_apply_args(mpv_handle *mpv_ctx, char *args);
static void mpv_log_handler(context_t *ctx, mpv_event_log_message* message);
static void seek_relative(context_t *ctx, gint offset);
static void resize_window_to_fit(context_t *ctx, gdouble multiplier);
static void mpv_init(context_t *ctx, gint64 vid_area_wid);
static void *mpv_event_handler(void *data);
static void destroy_handler(GtkWidget *widget, gpointer data);
static void open_handler(GtkWidget *widget, gpointer data);
static void open_loc_handler(GtkWidget *widget, gpointer data);
static void pref_handler(GtkWidget *widget, gpointer data);
static void play_handler(GtkWidget *widget, gpointer data);
static void stop_handler(GtkWidget *widget, gpointer data);
static void forward_handler(GtkWidget *widget, gpointer data);
static void rewind_handler(GtkWidget *widget, gpointer data);
static void chapter_previous_handler(GtkWidget *widget, gpointer data);
static void chapter_next_handler(GtkWidget *widget, gpointer data);
static void playlist_handler(GtkWidget *widget, gpointer data);
static void playlist_previous_handler(GtkWidget *widget, gpointer data);
static void playlist_next_handler(GtkWidget *widget, gpointer data);
static void fullscreen_handler(GtkWidget *widget, gpointer data);
static void normal_size_handler(GtkWidget *widget, gpointer data);
static void double_size_handler(GtkWidget *widget, gpointer data);
static void half_size_handler(GtkWidget *widget, gpointer data);
static void about_handler(GtkWidget *widget, gpointer data);
static void volume_handler(GtkWidget *widget, gpointer data);

static inline gchar *get_config_string(	context_t *ctx,
					const gchar *group,
					const gchar *key );

static inline void set_config_string(	context_t *ctx,
					const gchar *group,
					const gchar *key,
					const gchar *value );

static void mpv_load(	context_t *ctx,
			const gchar *uri,
			gboolean append,
			gboolean update );

static void window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );

static void playlist_row_handler(	GtkTreeView *tree_view,
					GtkTreePath *path,
					GtkTreeViewColumn *column,
					gpointer data );

static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data);

static void seek_handler(	GtkWidget *widget,
				GtkScrollType scroll,
				gdouble value,
				gpointer data );

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data );

static inline void mpv_check_error(int status)
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

static inline gchar *get_config_string(	context_t *ctx,
					const gchar *group,
					const gchar *key )
{
	return g_key_file_get_string(ctx->config_file, group, key, NULL);
}

static inline void set_config_string(	context_t *ctx,
					const gchar *group,
					const gchar *key,
					const gchar *value)
{
	g_key_file_set_string(ctx->config_file, group, key, value);
}

static inline gchar *get_config_file_path(void)
{
	return g_strconcat(	g_get_user_config_dir(),
				"/",
				CONFIG_FILE,
				NULL );
}

static gboolean update_seek_bar(gpointer data)
{
	context_t* ctx = data;
	gboolean exit_flag;
	gdouble time_pos;
	gint rc;

	pthread_mutex_lock(ctx->mpv_event_mutex);

	exit_flag = ctx->exit_flag;

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	if(!exit_flag)
	{
		pthread_mutex_lock(ctx->mpv_event_mutex);

		rc = mpv_get_property(	ctx->mpv_ctx,
					"time-pos",
					MPV_FORMAT_DOUBLE,
					&time_pos );

		pthread_mutex_unlock(ctx->mpv_event_mutex);

		if(rc >= 0)
		{
			gtk_range_set_value(	GTK_RANGE(ctx->gui->seek_bar),
						time_pos );
		}
	}

	return !exit_flag;
}

static gboolean reset_control(gpointer data)
{
	context_t* ctx = data;

	gtk_window_set_title(GTK_WINDOW(ctx->gui), g_get_application_name());
	main_window_reset_control(ctx->gui);

	return FALSE;
}

static gboolean reset_playlist(gpointer data)
{
	context_t *ctx = data;
	PlaylistWidget *playlist = PLAYLIST_WIDGET(ctx->gui->playlist);

	playlist_widget_set_indicator_pos(playlist, 0);

	return FALSE;
}

static gboolean mpv_load_gui_update(gpointer data)
{
	context_t* ctx = data;
	GtkWidget *icon;
	gchar* title;
	gint64 chapter_count;
	gint64 playlist_pos;
	gboolean new_file;
	gdouble length;
	gdouble volume;

	title = mpv_get_property_string(ctx->mpv_ctx, "media-title");

	icon = gtk_image_new_from_icon_name(	ctx->paused
						?"media-playback-start"
						:"media-playback-pause",
						GTK_ICON_SIZE_BUTTON );

	gtk_button_set_image(GTK_BUTTON(ctx->gui->play_button), icon);

	if(title)
	{
		gtk_window_set_title(GTK_WINDOW(ctx->gui), title);

		mpv_free(title);
	}

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
		if(chapter_count > 1)
		{
			main_window_show_chapter_control(ctx->gui);
		}
		else
		{
			main_window_hide_chapter_control(ctx->gui);
		}
	}

	if(mpv_get_property(	ctx->mpv_ctx,
				"volume",
				MPV_FORMAT_DOUBLE,
				&volume) >= 0)
	{
		gtk_scale_button_set_value
			(GTK_SCALE_BUTTON(ctx->gui->volume_button), volume/100);
	}

	if(mpv_get_property(	ctx->mpv_ctx,
				"length",
				MPV_FORMAT_DOUBLE,
				&length) >= 0)
	{
		main_window_set_seek_bar_length(ctx->gui, length);
	}

	pthread_mutex_lock(ctx->mpv_event_mutex);

	new_file = ctx->new_file;
	ctx->new_file = FALSE;

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	if(new_file)
	{
		resize_window_to_fit(ctx, 1);
	}

	return FALSE;
}

static gboolean mpv_load_from_ctx(gpointer data)
{
	context_t* ctx = data;

	mpv_load(ctx, NULL, FALSE, FALSE);

	return FALSE;
}

static gboolean show_error_dialog(gpointer data)
{
	context_t* ctx = data;

	if(ctx->log_buffer)
	{
		GtkWidget *dialog
			= gtk_message_dialog_new
				(	GTK_WINDOW(ctx->gui),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"Error" );

		gtk_message_dialog_format_secondary_text
			(GTK_MESSAGE_DIALOG(dialog), "%s", ctx->log_buffer);

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		g_free(ctx->log_buffer);

		ctx->log_buffer = NULL;
	}

	return FALSE;
}

static void seek_relative(context_t *ctx, gint offset)
{
	const gchar *cmd[] = {"seek", NULL, NULL};
	gchar *offset_str = g_strdup_printf("%d", offset);

	cmd[1] = offset_str;

	if(!ctx->loaded)
	{
		mpv_load(ctx, NULL, FALSE, TRUE);
	}

	mpv_check_error(mpv_command(ctx->mpv_ctx, cmd));

	g_free(offset_str);

	update_seek_bar(ctx);
}

static void resize_window_to_fit(context_t *ctx, gdouble multiplier)
{
	gchar *video = mpv_get_property_string(ctx->mpv_ctx, "video");
	gint64 width;
	gint64 height;
	gint mpv_width_rc;
	gint mpv_height_rc;

	mpv_width_rc = mpv_get_property(	ctx->mpv_ctx,
						"dwidth",
						MPV_FORMAT_INT64,
						&width );

	mpv_height_rc = mpv_get_property(	ctx->mpv_ctx,
						"dheight",
						MPV_FORMAT_INT64,
						&height );

	if(video
	&& strncmp(video, "no", 3) != 0
	&& mpv_width_rc >= 0
	&& mpv_height_rc >= 0)
	{
		gint width_margin
			= gtk_widget_get_allocated_width(GTK_WIDGET(ctx->gui))
			- gtk_widget_get_allocated_width(ctx->gui->vid_area);

		gint height_margin
			= gtk_widget_get_allocated_height(GTK_WIDGET(ctx->gui))
			- gtk_widget_get_allocated_height(ctx->gui->vid_area);

		gtk_window_resize(	GTK_WINDOW(ctx->gui),
					(multiplier*width)+width_margin,
					(multiplier*height)+height_margin );
	}

	mpv_free(video);
}

static void mpv_load(	context_t *ctx,
			const gchar *uri,
			gboolean append,
			gboolean update )
{
	const gchar *load_cmd[] = {"loadfile", NULL, NULL, NULL};
	GPtrArray *uri_list;
	guint uri_list_len;

	load_cmd[2] = append?"append":"replace";

	pthread_mutex_lock(ctx->mpv_event_mutex);

	uri_list = ctx->uri_list;
	uri_list_len = uri_list->len;

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	if(!append)
	{
		pthread_mutex_lock(ctx->mpv_event_mutex);

		playlist_widget_clear(PLAYLIST_WIDGET(ctx->gui->playlist));

		if(uri && update)
		{
			g_ptr_array_set_size(ctx->uri_list, 0);

			ctx->new_file = TRUE;
		}

		pthread_mutex_unlock(ctx->mpv_event_mutex);
	}

	if(!uri)
	{
		guint i;

		for(i = 0; i < uri_list_len; i++)
		{
			mpv_load(	ctx,
					g_ptr_array_index(uri_list, i),
					(i != 0),
					FALSE);
		}
	}

	if(uri && uri_list)
	{
		GFile *file;
		gchar *path;
		gchar *basename;
		const gchar *scheme;

		pthread_mutex_lock(ctx->mpv_event_mutex);

		if(!append)
		{
			ctx->loaded = FALSE;
		}

		if(update)
		{
			g_ptr_array_add(uri_list, g_strdup(uri));
		}

		pthread_mutex_unlock(ctx->mpv_event_mutex);

		scheme = g_uri_parse_scheme(uri);
		file = g_vfs_get_file_for_uri(g_vfs_get_default(), uri);
		path = g_file_get_path(file);

		/* If scheme is NULL then the uri is probably a local path */
		if(!scheme || path)
		{
			basename = g_path_get_basename(path?path:uri);
		}
		else
		{
			basename = NULL;
		}

		load_cmd[1] = path?path:uri;

		playlist_widget_append(	PLAYLIST_WIDGET(ctx->gui->playlist),
					basename?basename:uri );

		main_window_set_control_enabled(ctx->gui, TRUE);

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

		if(file)
		{
			g_object_unref(file);
		}

		g_free(path);
		g_free(basename);
	}
}

static gint mpv_apply_args(mpv_handle *mpv_ctx, gchar *args)
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

static void mpv_log_handler(context_t *ctx, mpv_event_log_message* message)
{
	const gchar *text = message->text;
	gchar *buffer;
	gboolean message_complete;
	gboolean log_complete;

	pthread_mutex_lock(ctx->mpv_event_mutex);

	buffer = ctx->log_buffer;

	message_complete
		= (text && text[strlen(text)-1] == '\n');

	log_complete
		= (buffer && buffer[strlen(buffer)-1] == '\n');

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

static void mpv_init(context_t *ctx, gint64 vid_area_wid)
{
	gchar *buffer = NULL;
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
						"screenshot-template",
						screenshot_template ));

	mpv_check_error(mpv_set_option(	ctx->mpv_ctx,
					"wid",
					MPV_FORMAT_INT64,
					&vid_area_wid ));

	mpv_check_error(mpv_request_log_messages(ctx->mpv_ctx, "error"));

	load_config(ctx);

	buffer = get_config_string(ctx, "main", "mpv-options");

	/* Apply extra options */
	if(mpv_apply_args(ctx->mpv_ctx, buffer) < 0)
	{
		ctx->log_buffer
			= g_strdup("Failed to apply one or more MPV options.");

		show_error_dialog(ctx);
	}

	mpv_check_error(mpv_initialize(ctx->mpv_ctx));

	g_free(buffer);
	g_free(screenshot_template);
}

static void *mpv_event_handler(void *data)
{
	context_t *ctx = data;
	gint exit_flag = FALSE;
	mpv_event *event;

	while(!exit_flag)
	{
		event = mpv_wait_event(ctx->mpv_ctx, 1);

		if(event->event_id == MPV_EVENT_IDLE)
		{
			pthread_mutex_lock(ctx->mpv_event_mutex);

			if(ctx->loaded)
			{
				ctx->paused = TRUE;
				ctx->loaded = FALSE;

				g_idle_add((GSourceFunc)reset_control, ctx);
				g_idle_add((GSourceFunc)reset_playlist, ctx);
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

static void destroy_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = data;

	pthread_mutex_lock(ctx->mpv_event_mutex);

	ctx->exit_flag = TRUE;

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	pthread_join(*(ctx->mpv_event_handler_thread), NULL);

	pthread_mutex_destroy(ctx->mpv_event_mutex);
	pthread_cond_destroy(ctx->mpv_ctx_init_cv);
	pthread_cond_destroy(ctx->mpv_ctx_destroy_cv);

	mpv_terminate_destroy(ctx->mpv_ctx);
	g_key_file_free(ctx->config_file);

	gtk_main_quit();
}

static void open_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t*)data;
	GtkWidget *open_dialog;

	open_dialog
		= gtk_file_chooser_dialog_new(	"Open File",
						GTK_WINDOW(ctx->gui),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						"_Cancel",
						GTK_RESPONSE_CANCEL,
						"_Open",
						GTK_RESPONSE_ACCEPT,
						NULL );

	if(gtk_dialog_run(GTK_DIALOG(open_dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER(open_dialog);
		gchar *uri = gtk_file_chooser_get_filename(chooser);

		ctx->paused = FALSE;

		mpv_load(ctx, uri, FALSE, TRUE);

		g_free(uri);
	}

	gtk_widget_destroy(open_dialog);
}

static void open_loc_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t*)data;
	OpenLocDialog *open_loc_dialog;

	open_loc_dialog
		= OPEN_LOC_DIALOG(open_loc_dialog_new(GTK_WINDOW(ctx->gui)));

	if(	gtk_dialog_run(GTK_DIALOG(open_loc_dialog))
		== GTK_RESPONSE_ACCEPT )
	{
		const gchar *loc_str;

		loc_str = open_loc_dialog_get_string(open_loc_dialog);

		ctx->paused = FALSE;

		mpv_load(ctx, loc_str, FALSE, TRUE);
	}

	gtk_widget_destroy(GTK_WIDGET(open_loc_dialog));
}

static gboolean load_config(context_t *ctx)
{
	gboolean result;
	gchar *path = get_config_file_path();

	pthread_mutex_lock(ctx->mpv_event_mutex);

	result = g_key_file_load_from_file(	ctx->config_file,
						path,
						G_KEY_FILE_KEEP_COMMENTS,
						NULL );

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	g_free(path);

	return result;
}

static gboolean save_config(context_t *ctx)
{
	gboolean result;
	gchar *path = get_config_file_path();

	pthread_mutex_lock(ctx->mpv_event_mutex);

	result = g_key_file_save_to_file(ctx->config_file, path, NULL);

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	return result;
}

static void pref_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = data;
	PrefDialog *pref_dialog;
	gchar *buffer;
	const gchar *quit_cmd[] = {"quit_watch_later", NULL};

	load_config(ctx);

	buffer = get_config_string(ctx, "main", "mpv-options");

	pref_dialog = PREF_DIALOG(pref_dialog_new(GTK_WINDOW(ctx->gui)));

	if(buffer)
	{
		pref_dialog_set_string(pref_dialog, buffer);

		g_free(buffer);
	}

	if(gtk_dialog_run(GTK_DIALOG(pref_dialog))
	== GTK_RESPONSE_ACCEPT)
	{
		gint64 playlist_pos;
		gdouble time_pos;
		gint playlist_pos_rc;
		gint time_pos_rc;

		set_config_string(	ctx,
					"main",
					"mpv-options",
					pref_dialog_get_string(pref_dialog) );

		save_config(ctx);

		mpv_check_error(mpv_set_property_string(	ctx->mpv_ctx,
								"pause",
								"yes" ));

		playlist_pos_rc = mpv_get_property(	ctx->mpv_ctx,
							"playlist-pos",
							MPV_FORMAT_INT64,
							&playlist_pos );

		time_pos_rc = mpv_get_property(	ctx->mpv_ctx,
						"time-pos",
						MPV_FORMAT_DOUBLE,
						&time_pos );

		/* Suspend mpv_event_handler loop */
		pthread_mutex_lock(ctx->mpv_event_mutex);

		ctx->mpv_ctx_reset = TRUE;

		pthread_cond_wait(	ctx->mpv_ctx_destroy_cv,
					ctx->mpv_event_mutex );

		pthread_mutex_unlock(ctx->mpv_event_mutex);

		/* Reset ctx->mpv_ctx */
		mpv_check_error(mpv_command(ctx->mpv_ctx, quit_cmd));

		mpv_detach_destroy(ctx->mpv_ctx);

		ctx->mpv_ctx = mpv_create();

		mpv_init(ctx, ctx->vid_area_wid);

		/* Wake up mpv_event_handler loop */
		pthread_mutex_lock(ctx->mpv_event_mutex);

		ctx->mpv_ctx_reset = FALSE;

		pthread_cond_signal(ctx->mpv_ctx_init_cv);

		pthread_mutex_unlock(ctx->mpv_event_mutex);

		if(ctx->uri_list)
		{
			gint rc;

			rc = mpv_request_event(	ctx->mpv_ctx,
						MPV_EVENT_FILE_LOADED,
						0 );

			mpv_check_error(rc);

			mpv_load(ctx, NULL, FALSE, TRUE);

			rc = mpv_request_event(	ctx->mpv_ctx,
						MPV_EVENT_FILE_LOADED,
						1 );

			mpv_check_error(rc);

			if(playlist_pos_rc >= 0 && playlist_pos > 0)
			{
				mpv_set_property(	ctx->mpv_ctx,
							"playlist-pos",
							MPV_FORMAT_INT64,
							&playlist_pos );
			}

			if(time_pos_rc >= 0 && time_pos > 0)
			{
				mpv_set_property(	ctx->mpv_ctx,
							"time-pos",
							MPV_FORMAT_DOUBLE,
							&time_pos );
			}

			mpv_set_property(	ctx->mpv_ctx,
						"pause",
						MPV_FORMAT_FLAG,
						&ctx->paused );
		}
	}

	gtk_widget_destroy(GTK_WIDGET(pref_dialog));
}

static void play_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = data;
	GtkWidget *icon;
	gboolean loaded;

	pthread_mutex_lock(ctx->mpv_event_mutex);

	ctx->paused = !ctx->paused;
	loaded = ctx->loaded;

	pthread_mutex_unlock(ctx->mpv_event_mutex);

	if(!loaded)
	{
		mpv_load(ctx, NULL, FALSE, TRUE);
	}

	mpv_check_error(mpv_set_property(	ctx->mpv_ctx,
						"pause",
						MPV_FORMAT_FLAG,
						&ctx->paused ));

	icon = gtk_image_new_from_icon_name(	ctx->paused
						?"media-playback-start"
						:"media-playback-pause",
						GTK_ICON_SIZE_BUTTON );

	gtk_button_set_image(GTK_BUTTON(ctx->gui->play_button), icon);
}

static void stop_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t *)data;
	const gchar *seek_cmd[] = {"seek", "0", "absolute", NULL};

	mpv_check_error(mpv_set_property_string(ctx->mpv_ctx, "pause", "yes"));
	mpv_check_error(mpv_command(ctx->mpv_ctx, seek_cmd));

	ctx->paused = TRUE;

	reset_control(ctx);
	update_seek_bar(ctx);
}

static void forward_handler(GtkWidget *widget, gpointer data)
{
	seek_relative(data, 10);
}

static void rewind_handler(GtkWidget *widget, gpointer data)
{
	seek_relative(data, -10);
}

static void chapter_previous_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t *)data;
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", "down", NULL};

	if(!ctx->loaded)
	{
		mpv_load(ctx, NULL, FALSE, TRUE);
	}

	mpv_check_error(mpv_command(ctx->mpv_ctx, cmd));
}

static void chapter_next_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t *)data;
	const gchar *cmd[] = {"osd-msg", "cycle", "chapter", NULL};

	if(!ctx->loaded)
	{
		mpv_load(ctx, NULL, FALSE, TRUE);
	}

	mpv_check_error(mpv_command(ctx->mpv_ctx, cmd));
}

static void playlist_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = data;
	gboolean visible = gtk_widget_get_visible(ctx->gui->playlist);

	main_window_set_playlist_visible(ctx->gui, !visible);
}

static void playlist_previous_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t *)data;
	const gchar *cmd[] = {"playlist_prev", NULL};

	mpv_command(ctx->mpv_ctx, cmd);
}

static void playlist_next_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t *)data;
	const gchar *cmd[] = {"playlist_next", NULL};

	mpv_command(ctx->mpv_ctx, cmd);
}

static void playlist_row_handler(	GtkTreeView *tree_view,
					GtkTreePath *path,
					GtkTreeViewColumn *column,
					gpointer data )
{
	context_t *ctx = data;
	gint *indices = gtk_tree_path_get_indices(path);

	if(indices)
	{
		gint64 index = indices[0];

		mpv_set_property(	ctx->mpv_ctx,
					"playlist-pos",
					MPV_FORMAT_INT64,
					&index );
	}
}

static void fullscreen_handler(GtkWidget *widget, gpointer data)
{
	main_window_toggle_fullscreen(((context_t *)data)->gui);
}

static void normal_size_handler(GtkWidget *widget, gpointer data)
{
	resize_window_to_fit((context_t *)data, 1);
}

static void double_size_handler(GtkWidget *widget, gpointer data)
{
	resize_window_to_fit((context_t *)data, 2);
}

static void half_size_handler(GtkWidget *widget, gpointer data)
{
	resize_window_to_fit((context_t *)data, 0.5);
}

static void about_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = (context_t*)data;

	gtk_show_about_dialog(	GTK_WINDOW(ctx->gui),
				"logo-icon-name",
				ICON_NAME,
				"version",
				APP_VERSION,
				"comments",
				APP_DESC,
				"license-type",
				GTK_LICENSE_GPL_3_0,
				NULL );
}

static void volume_handler(GtkWidget *widget, gpointer data)
{
	context_t *ctx = data;
	gdouble value;

	g_object_get(widget, "value", &value, NULL);

	value *= 100;

	mpv_set_property(ctx->mpv_ctx, "volume", MPV_FORMAT_DOUBLE, &value);
}

static void window_state_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	GdkWindowState window_state
		= ((GdkEventWindowState *)event)->new_window_state;

	*((gint *)data)
		= ((window_state&GDK_WINDOW_STATE_FULLSCREEN) != 0);
}

static void drag_data_handler(	GtkWidget *widget,
				GdkDragContext *context,
				gint x,
				gint y,
				GtkSelectionData *sel_data,
				guint info,
				guint time,
				gpointer data)
{
	if(sel_data && gtk_selection_data_get_length(sel_data) > 0)
	{
		context_t *ctx = data;
		gchar **uri_list = gtk_selection_data_get_uris(sel_data);

		ctx->paused = FALSE;

		if(uri_list)
		{
			int i;

			for(i = 0; uri_list[i]; i++)
			{
				mpv_load(ctx, uri_list[i], (i != 0), TRUE);
			}

			g_strfreev(uri_list);
		}
		else
		{
			const guchar *raw_data
				= gtk_selection_data_get_data(sel_data);

			mpv_load(ctx, (const gchar *)raw_data, FALSE, TRUE);
		}
	}
}

static void seek_handler(	GtkWidget *widget,
				GtkScrollType scroll,
				gdouble value,
				gpointer data )
{
	context_t *ctx = data;
	const gchar *cmd[] = {"seek", NULL, "absolute", NULL};
	gchar *value_str = g_strdup_printf("%.2f", value);

	cmd[1] = value_str;

	if(!ctx->loaded)
	{
		mpv_load(ctx, NULL, FALSE, TRUE);
	}

	mpv_check_error(mpv_command(ctx->mpv_ctx, cmd));

	g_free(value_str);
}

static gboolean key_press_handler(	GtkWidget *widget,
					GdkEvent *event,
					gpointer data )
{
	context_t *ctx = data;
	guint keyval = ((GdkEventKey*)event)->keyval;
	guint state = ((GdkEventKey*)event)->state;

	const guint mod_mask =	GDK_MODIFIER_MASK
				&~(GDK_SHIFT_MASK
				|GDK_LOCK_MASK
				|GDK_MOD2_MASK
				|GDK_MOD3_MASK
				|GDK_MOD4_MASK
				|GDK_MOD5_MASK);

	/* Make sure that no modifier key is active except certain keys like
	 * caps lock.
	 */
	if((state&mod_mask) == 0)
	{
		/* Accept F11 (via accelerator) and f for entering/exiting
		 * fullscreen mode. ESC is only used for exiting fullscreen
		 * mode.
		 */
		if((ctx->gui->fullscreen
		&& (keyval == GDK_KEY_F11 || keyval == GDK_KEY_Escape))
		|| keyval == GDK_KEY_f)
		{
			fullscreen_handler(NULL, ctx);
		}
		else if(keyval == GDK_KEY_v)
		{
			const gchar *cmd[] = {	"osd-msg",
						"cycle",
						"sub-visibility",
						NULL };

			mpv_command(ctx->mpv_ctx, cmd);
		}
		else if(keyval == GDK_KEY_s)
		{
			const gchar *cmd[] = {	"osd-msg",
						"screenshot",
						NULL };

			mpv_command(ctx->mpv_ctx, cmd);
		}
		else if(keyval == GDK_KEY_S)
		{
			const gchar *cmd[] = {	"osd-msg",
						"screenshot",
						"video",
						NULL };

			mpv_command(ctx->mpv_ctx, cmd);
		}
		else if(keyval == GDK_KEY_j)
		{
			const gchar *cmd[] = {	"osd-msg",
						"cycle",
						"sub",
						NULL };

			mpv_command(ctx->mpv_ctx, cmd);
		}
		else if(keyval == GDK_KEY_J)
		{
			const gchar *cmd[] = {	"osd-msg",
						"cycle",
						"sub",
						"down",
						NULL };

			mpv_command(ctx->mpv_ctx, cmd);
		}
		else if(keyval == GDK_KEY_Left)
		{
			seek_relative(ctx, -10);
		}
		else if(keyval == GDK_KEY_Right)
		{
			seek_relative(ctx, 10);
		}
		else if(keyval == GDK_KEY_Up)
		{
			seek_relative(ctx, 60);
		}
		else if(keyval == GDK_KEY_Down)
		{
			seek_relative(ctx, -60);
		}
		else if(keyval == GDK_KEY_space || keyval == GDK_KEY_p)
		{
			play_handler(NULL, ctx);
		}
		else if(keyval == GDK_KEY_U)
		{
			stop_handler(NULL, ctx);
		}
		else if(keyval == GDK_KEY_exclam)
		{
			chapter_previous_handler(NULL, ctx);
		}
		else if(keyval == GDK_KEY_at)
		{
			chapter_next_handler(NULL, ctx);
		}
		else if(keyval == GDK_KEY_less)
		{
			playlist_previous_handler(NULL, ctx);
		}
		else if(keyval == GDK_KEY_greater)
		{
			playlist_next_handler(NULL, ctx);
		}
	}

	return FALSE;
}

int main(int argc, char **argv)
{
	context_t ctx;
	GtkTargetEntry target_entry[3];
	pthread_t mpv_event_handler_thread;
	pthread_mutex_t mpv_event_mutex;
	pthread_cond_t mpv_ctx_init_cv;
	pthread_cond_t mpv_ctx_destroy_cv;

	gtk_init(&argc, &argv);
	g_set_application_name(APP_NAME);
	gtk_window_set_default_icon_name(ICON_NAME);

	ctx.mpv_ctx = mpv_create();
	ctx.exit_flag = FALSE;
	ctx.mpv_ctx_reset = FALSE;
	ctx.paused = TRUE;
	ctx.loaded = FALSE;
	ctx.new_file = TRUE;
	ctx.sub_visible = TRUE;
	ctx.uri_list = g_ptr_array_new_with_free_func((GDestroyNotify)g_free);
	ctx.log_buffer = NULL;
	ctx.config_file = g_key_file_new();
	ctx.gui = MAIN_WINDOW(main_window_new());
	ctx.mpv_event_handler_thread = &mpv_event_handler_thread;
	ctx.mpv_ctx_init_cv = &mpv_ctx_init_cv;
	ctx.mpv_ctx_destroy_cv = &mpv_ctx_destroy_cv;
	ctx.mpv_event_mutex = &mpv_event_mutex;
	ctx.mpv_event_mutex = &mpv_event_mutex;

	ctx.vid_area_wid = gdk_x11_window_get_xid
				(gtk_widget_get_window(ctx.gui->vid_area));

	target_entry[0].target = "text/uri-list";
	target_entry[0].flags = 0;
	target_entry[0].info = 0;
	target_entry[1].target = "text/plain";
	target_entry[1].flags = 0;
	target_entry[1].info = 1;
	target_entry[2].target = "STRING";
	target_entry[2].flags = 0;
	target_entry[2].info = 1;

	pthread_mutex_init(ctx.mpv_event_mutex, NULL);
	pthread_cond_init(ctx.mpv_ctx_init_cv, NULL);
	pthread_cond_init(ctx.mpv_ctx_destroy_cv, NULL);

	gtk_drag_dest_set(	GTK_WIDGET(ctx.gui),
				GTK_DEST_DEFAULT_ALL,
				target_entry,
				3,
				GDK_ACTION_COPY );

	gtk_drag_dest_add_uri_targets(GTK_WIDGET(ctx.gui));
	gtk_range_set_increments(GTK_RANGE(ctx.gui->seek_bar), 10, 10);

	g_signal_connect(	ctx.gui,
				"drag-data-received",
				G_CALLBACK(drag_data_handler),
				&ctx );

	g_signal_connect(	ctx.gui,
				"destroy",
				G_CALLBACK(destroy_handler),
				&ctx );

	g_signal_connect(	ctx.gui,
				"window-state-event",
				G_CALLBACK(window_state_handler),
				&ctx.gui->fullscreen );

	g_signal_connect(	ctx.gui,
				"key-press-event",
				G_CALLBACK(key_press_handler),
				&ctx );

	g_signal_connect(	PLAYLIST_WIDGET(ctx.gui->playlist)->tree_view,
				"row-activated",
				G_CALLBACK(playlist_row_handler),
				&ctx );

	g_signal_connect(	ctx.gui->play_button,
				"clicked",
				G_CALLBACK(play_handler),
				&ctx );

	g_signal_connect(	ctx.gui->stop_button,
				"clicked",
				G_CALLBACK(stop_handler),
				&ctx );

	g_signal_connect(	ctx.gui->forward_button,
				"clicked",
				G_CALLBACK(forward_handler),
				&ctx );

	g_signal_connect(	ctx.gui->rewind_button,
				"clicked",
				G_CALLBACK(rewind_handler),
				&ctx );

	g_signal_connect(	ctx.gui->previous_button,
				"clicked",
				G_CALLBACK(chapter_previous_handler),
				&ctx );

	g_signal_connect(	ctx.gui->next_button,
				"clicked",
				G_CALLBACK(chapter_next_handler),
				&ctx );

	g_signal_connect(	ctx.gui->fullscreen_button,
				"clicked",
				G_CALLBACK(fullscreen_handler),
				&ctx );

	g_signal_connect(	ctx.gui->open_menu_item,
				"activate",
				G_CALLBACK(open_handler),
				&ctx );

	g_signal_connect(	ctx.gui->open_loc_menu_item,
				"activate",
				G_CALLBACK(open_loc_handler),
				&ctx );

	g_signal_connect(	ctx.gui->quit_menu_item,
				"activate",
				G_CALLBACK(destroy_handler),
				&ctx );

	g_signal_connect(	ctx.gui->pref_menu_item,
				"activate",
				G_CALLBACK(pref_handler),
				&ctx );

	g_signal_connect(	ctx.gui->playlist_menu_item,
				"activate",
				G_CALLBACK(playlist_handler),
				&ctx );

	g_signal_connect(	ctx.gui->fullscreen_menu_item,
				"activate",
				G_CALLBACK(fullscreen_handler),
				&ctx );

	g_signal_connect(	ctx.gui->normal_size_menu_item,
				"activate",
				G_CALLBACK(normal_size_handler),
				&ctx );

	g_signal_connect(	ctx.gui->double_size_menu_item,
				"activate",
				G_CALLBACK(double_size_handler),
				&ctx );

	g_signal_connect(	ctx.gui->half_size_menu_item,
				"activate",
				G_CALLBACK(half_size_handler),
				&ctx );

	g_signal_connect(	ctx.gui->about_menu_item,
				"activate",
				G_CALLBACK(about_handler),
				&ctx );

	g_signal_connect(	ctx.gui->volume_button,
				"value-changed",
				G_CALLBACK(volume_handler),
				&ctx );

	g_signal_connect(	ctx.gui->seek_bar,
				"change-value",
				G_CALLBACK(seek_handler),
				&ctx );

	mpv_init(&ctx, ctx.vid_area_wid);

	pthread_create(	&mpv_event_handler_thread,
			NULL,
			mpv_event_handler,
			&ctx );

	g_timeout_add(	SEEK_BAR_UPDATE_INTERVAL,
			(GSourceFunc)update_seek_bar,
			&ctx );

	/* Start playing the file given as command line argument, if any */
	if(argc >= 2)
	{
		gint i = 0;

		pthread_mutex_lock(ctx.mpv_event_mutex);

		ctx.paused = FALSE;

		for(i = 1; i < argc; i++)
		{
			g_ptr_array_add(ctx.uri_list, g_strdup(argv[i]));
		}

		pthread_mutex_unlock(ctx.mpv_event_mutex);

		g_idle_add((GSourceFunc)mpv_load_from_ctx, &ctx);
	}
	else
	{
		main_window_set_control_enabled(ctx.gui, FALSE);
	}

	gtk_main();

	return EXIT_SUCCESS;
}
