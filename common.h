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

#ifndef COMMON_H
#define COMMON_H

#include <mpv/client.h>

#include "main_window.h"

typedef struct gmpv_handle gmpv_handle;

struct gmpv_handle
{
	mpv_handle *mpv_ctx;
	volatile gboolean exit_flag;
	volatile gboolean mpv_ctx_reset;
	gboolean paused;
	gboolean loaded;
	gboolean new_file;
	gboolean sub_visible;
	gint64 vid_area_wid;
	gint playlist_move_dest;
	gchar *log_buffer;
	gchar *mpv_options;
	GtkListStore *playlist_store;
	GKeyFile *config_file;
	GtkWidget *fs_control;
	MainWindow *gui;
	pthread_t *mpv_event_handler_thread;
	pthread_mutex_t *mpv_event_mutex;
	pthread_cond_t *mpv_ctx_init_cv;
	pthread_cond_t *mpv_ctx_destroy_cv;
};

inline gchar *get_config_string(	gmpv_handle *ctx,
					const gchar *group,
					const gchar *key );

inline void set_config_string(	gmpv_handle *ctx,
					const gchar *group,
					const gchar *key,
					const gchar *value );

inline gchar *get_config_file_path(void);
gboolean load_config(gmpv_handle *ctx);
gboolean save_config(gmpv_handle *ctx);
gchar *get_path_from_uri(const gchar *uri);
gchar *get_name_from_path(const gchar *path);
gboolean update_seek_bar(gpointer data);
gboolean control_reset(gpointer data);
gboolean show_error_dialog(gpointer data);
void seek_relative(gmpv_handle *ctx, gint offset);
void resize_window_to_fit(gmpv_handle *ctx, gdouble multiplier);

#endif
