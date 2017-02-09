/*
 * Copyright (c) 2017 gnome-mpv
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

#ifndef FILE_CHOOSER_H
#define FILE_CHOOSER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#if GTK_CHECK_VERSION(3, 19, 7)
#define GmpvFileChooser GtkFileChooserNative
#define gmpv_file_chooser_destroy(x) gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(x))
#define gmpv_file_chooser_show(x) gtk_native_dialog_show(GTK_NATIVE_DIALOG(x))
#define gmpv_file_chooser_set_modal(x, y) gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(x), y)
#define gmpv_file_chooser_run(x) gtk_native_dialog_run(GTK_NATIVE_DIALOG(x))
#else
#define GmpvFileChooser GtkWidget
#define gmpv_file_chooser_destroy(x) gtk_widget_destroy(GTK_WIDGET(x))
#define gmpv_file_chooser_show(x) gtk_widget_show_all(GTK_DIALOG(x))
#define gmpv_file_chooser_set_modal(x, y) gtk_window_set_modal(GTK_WINDOW(x), y)
#define gmpv_file_chooser_run(x) gtk_dialog_run(GTK_DIALOG(x))
#endif

GmpvFileChooser *gmpv_file_chooser_new(	const gchar *title,
					GtkWindow *parent,
					GtkFileChooserAction action );

G_END_DECLS

#endif
