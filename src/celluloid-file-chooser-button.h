/*
 * Copyright (c) 2021 gnome-mpv
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

#ifndef FILE_CHOOSER_BUTTON_H
#define FILE_CHOOSER_BUTTON_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_FILE_CHOOSER_BUTTON (celluloid_file_chooser_button_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidFileChooserButton, celluloid_file_chooser_button, CELLULOID, FILE_CHOOSER_BUTTON, GtkButton)

GtkWidget *
celluloid_file_chooser_button_new(	const gchar *title,
					GtkFileChooserAction action );

void
celluloid_file_chooser_button_set_file(	CelluloidFileChooserButton *self,
					GFile *file,
					GError **error );

GFile *
celluloid_file_chooser_button_get_file(CelluloidFileChooserButton *self);

void
celluloid_file_chooser_button_set_filter(	CelluloidFileChooserButton *self,
						GtkFileFilter *filter );

G_END_DECLS

#endif
