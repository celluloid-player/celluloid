/*
 * Copyright (c) 2014-2015 gnome-mpv
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

#ifndef COMMON_H
#define COMMON_H

#include <mpv/client.h>

#include "main_window.h"

typedef struct gmpv_handle gmpv_handle;

struct gmpv_handle
{
	mpv_handle *mpv_ctx;
	gchar **files;
	gboolean paused;
	gboolean loaded;
	gboolean new_file;
	gboolean sub_visible;
	gboolean init_load;
	gint64 vid_area_wid;
	gint playlist_move_dest;
	gchar *log_buffer;
	GSList *keybind_list;
	GSettings *config;
	GtkApplication *app;
	MainWindow *gui;
	GtkWidget *fs_control;
	GtkListStore *playlist_store;
};

gchar *get_config_dir_path(void);
gchar *get_config_file_path(void);
gchar *get_path_from_uri(const gchar *uri);
gchar *get_name_from_path(const gchar *path);
gboolean quit(gpointer data);
gboolean update_seek_bar(gpointer data);
gboolean migrate_config(gmpv_handle *ctx);
void show_error_dialog(gmpv_handle *ctx);
void remove_current_playlist_entry(gmpv_handle *ctx);
void resize_window_to_fit(gmpv_handle *ctx, gdouble multiplier);
void toggle_fullscreen(gmpv_handle *ctx);
GMenu *build_full_menu(void);
void load_keybind(	gmpv_handle *ctx,
			const gchar *config_path,
			gboolean notify_ignore );

#endif
