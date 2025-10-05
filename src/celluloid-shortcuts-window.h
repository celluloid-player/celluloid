/*
 * Copyright (c) 2016-2021, 2025 gnome-mpv
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

#ifndef SHORTCUTS_WINDOW_H
#define SHORTCUTS_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef AdwShortcutsDialog CelluloidShortcutsWindow;

#define CELLULOID_SHORTCUTS_WINDOW ADW_SHORTCUTS_DIALOG
#define CELLULOID_TYPE_SHORTCUTS_WINDOW ADW_TYPE_SHORTCUTS_DIALOG
#define CELLULOID_IS_SHORTCUTS_WINDOW ADW_IS_SHORTCUTS_DIALOG

GtkWidget *
celluloid_shortcuts_window_new(GtkWindow *parent);

G_END_DECLS

#endif
