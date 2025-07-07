/*
 * Copyright (c) 2025 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CELLULOID_FILE_DIALOG GTK_FILE_DIALOG
#define CelluloidFileDialog GtkFileDialog
#define celluloid_file_dialog_set_filters(self, filters) gtk_file_dialog_set_filters(self, filters)
#define celluloid_file_dialog_set_default_filter(self, filter) gtk_file_dialog_set_default_filter(self, filter)
#define celluloid_file_dialog_set_title(self, title) gtk_file_dialog_set_title(self, title)
#define celluloid_file_dialog_set_initial_name(self, name) gtk_file_dialog_set_initial_name(self, name)
#define celluloid_file_dialog_set_initial_file(self, file) gtk_file_dialog_set_initial_file(self, file)
#define celluloid_file_dialog_get_initial_file(self) gtk_file_dialog_get_initial_file(self)
#define celluloid_file_dialog_open(self, parent, cancellable, callback, data) gtk_file_dialog_open(self, parent, cancellable, callback, data)
#define celluloid_file_dialog_open_multiple(self, parent, cancellable, callback, data) gtk_file_dialog_open_multiple(self, parent, cancellable, callback, data)
#define celluloid_file_dialog_select_folder(self, parent, cancellable, callback, data) gtk_file_dialog_select_folder(self, parent, cancellable, callback, data)
#define celluloid_file_dialog_save(self, parent, cancellable, callback, data) gtk_file_dialog_save(self, parent, cancellable, callback, data)

void
celluloid_file_dialog_set_default_filters(	CelluloidFileDialog *self,
						gboolean audio,
						gboolean video,
						gboolean image,
						gboolean subtitle );

GFile *
celluloid_file_dialog_open_finish(	GtkFileDialog *self,
					GAsyncResult *async_result,
					GError **error );

GListModel *
celluloid_file_dialog_open_multiple_finish(	GtkFileDialog *self,
						GAsyncResult *async_result,
						GError **error );

GFile *
celluloid_file_dialog_select_folder_finish(	GtkFileDialog *self,
						GAsyncResult *async_result,
						GError **error );

GFile *
celluloid_file_dialog_save_finish(	GtkFileDialog *self,
					GAsyncResult *async_result,
					GError **error );

CelluloidFileDialog *
celluloid_file_dialog_new(gboolean restore_state);

G_END_DECLS

#endif
