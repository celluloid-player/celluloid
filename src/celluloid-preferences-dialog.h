/*
 * Copyright (c) 2014-2019 gnome-mpv
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

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <adwaita.h>

G_BEGIN_DECLS

#define CELLULOID_TYPE_PREFERENCES_DIALOG (celluloid_preferences_dialog_get_type ())

G_DECLARE_FINAL_TYPE(CelluloidPreferencesDialog, celluloid_preferences_dialog, CELLULOID, PREFERENCES_DIALOG, AdwPreferencesWindow)

GtkWidget *
celluloid_preferences_dialog_new(GtkWindow *parent);

G_END_DECLS

#endif
