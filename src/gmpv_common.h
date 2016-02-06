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

#include "gmpv_playlist.h"
#include "gmpv_application.h"

G_BEGIN_DECLS

gchar *get_config_dir_path(void);
gchar *get_scripts_dir_path(void);
gchar *get_path_from_uri(const gchar *uri);
gchar *get_name_from_path(const gchar *path);
gboolean quit(gpointer data);
gboolean update_seek_bar(gpointer data);
void activate_action_string(GmpvApplication *app, const gchar *str);
void migrate_config(GmpvApplication *app);
void seek(GmpvApplication *app, gdouble time);
void show_error_dialog(GmpvApplication *app, const gchar *prefix, const gchar *msg);
void resize_window_to_fit(GmpvApplication *app, gdouble multiplier);
void load_keybind(	GmpvApplication *app,
			const gchar *config_path,
			gboolean notify_ignore );

#if GTK_CHECK_VERSION(3, 19, 7)
#define GmpvFileChooser GtkFileChooserNative
#define gmpv_file_chooser_destroy(x) gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(x))
#define gmpv_file_chooser_run(x) gtk_native_dialog_run(GTK_NATIVE_DIALOG(x))
#define gmpv_file_chooser_new(title,parent,action,accept,cancel) \
	gtk_file_chooser_native_new(title,parent,action,accept,cancel)
#else
#define GmpvFileChooser GtkWidget
#define gmpv_file_chooser_destroy(x) gtk_widget_destroy(GTK_WIDGET(x))
#define gmpv_file_chooser_run(x) gtk_dialog_run(GTK_DIALOG(x))
#define gmpv_file_chooser_new(title,parent,action,accept,cancel) \
	gtk_file_chooser_dialog_new(title,parent,action,cancel,\
				    GTK_RESPONSE_CANCEL,accept,GTK_RESPONSE_ACCEPT,NULL)
#endif

G_END_DECLS

#endif
