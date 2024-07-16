/*
 * Copyright (c) 2017-2019, 2021, 2024 gnome-mpv
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

#ifndef FILE_CHOOSER_H
#define FILE_CHOOSER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CELLULOID_FILE_CHOOSER GTK_FILE_CHOOSER_NATIVE
#define CelluloidFileChooser GtkFileChooserNative
#define celluloid_file_chooser_show(x) gtk_native_dialog_show(GTK_NATIVE_DIALOG(x))
#define celluloid_file_chooser_set_modal(x, y) gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(x), y)

// Ignore deprecation warnings for GtkFileChooser. As of 2024-07-16, we can't
// switch to GtkFileDialog because it uses the file portal by default, and
// it doesn't seem to respect per-application dark mode settings at this time.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

GFile *
celluloid_file_chooser_get_file(CelluloidFileChooser *chooser);

GListModel *
celluloid_file_chooser_get_files(CelluloidFileChooser *chooser);

gboolean
celluloid_file_chooser_set_file(	CelluloidFileChooser *chooser,
					GFile *file,
					GError **error );

void
celluloid_file_chooser_add_filter(	CelluloidFileChooser *chooser,
					GtkFileFilter *filter );

void
celluloid_file_chooser_set_filter(	CelluloidFileChooser *chooser,
					GtkFileFilter *filter );

void
celluloid_file_chooser_set_current_name(	CelluloidFileChooser *chooser,
						const gchar *name );

CelluloidFileChooser *
celluloid_file_chooser_new(	const gchar *title,
				GtkWindow *parent,
				GtkFileChooserAction action,
				gboolean restore_state );

void
celluloid_file_chooser_destroy(CelluloidFileChooser *chooser);

void
celluloid_file_chooser_set_default_filters(	CelluloidFileChooser *chooser,
						gboolean audio,
						gboolean video,
						gboolean image,
						gboolean subtitle );

#pragma GCC diagnostic pop

G_END_DECLS

#endif
