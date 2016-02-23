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

#ifndef COMMON_H
#define COMMON_H

#include <mpv/client.h>
#include <mpv/opengl_cb.h>

#include "playlist.h"
#include "application.h"

typedef struct module_log_level module_log_level;

struct module_log_level
{
	gchar *prefix;
	mpv_log_level level;
};

gchar *get_config_dir_path(void);
gchar *get_path_from_uri(const gchar *uri);
gchar *get_name_from_path(const gchar *path);
gboolean quit(gpointer data);
gboolean update_seek_bar(gpointer data);
void migrate_config(Application *app);
void seek(Application *app, gdouble time);
void show_error_dialog(Application *app, const gchar *prefix, const gchar *msg);
void resize_window_to_fit(Application *app, gdouble multiplier);
void toggle_fullscreen(Application *app);
void load_keybind(	Application *app,
			const gchar *config_path,
			gboolean notify_ignore );

#endif
