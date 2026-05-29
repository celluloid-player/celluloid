/*
 * Copyright (c) 2026 gnome-mpv
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

#ifndef FILE_H
#define FILE_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_FILE (celluloid_file_get_type())

G_DECLARE_FINAL_TYPE(CelluloidFile, celluloid_file, CELLULOID, FILE, GObject)

CelluloidFile *
celluloid_file_new_for_uri(const gchar *uri);

CelluloidFile *
celluloid_file_new_for_gfile(GFile *gfile);

GFile *
celluloid_file_get_gfile(CelluloidFile *file);

gchar *
celluloid_file_get_path(CelluloidFile *file);

gchar *
celluloid_file_get_uri(CelluloidFile *file);

G_END_DECLS

#endif
